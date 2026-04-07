#include "Hazel/Renderer/Renderer.h"

#include "GPUAsset/GPUGraphicsPipelineAsset.h"
#include "Hazel/Asset/ComputeShaderAsset.h"
#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Asset/RenderTextureAsset.h"
#include "Hazel/Asset/SamplerAsset.h"
#include "Hazel/Asset/ShaderAsset.h"
#include "Hazel/Asset/TextureAsset.h"
#include "Hazel/Project/GlobalSettingRegistry.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Renderer/GPUAsset/GPUComputeShaderAsset.h"
#include "Hazel/Renderer/GPUAsset/GPURenderTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUSamplerAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUShaderAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/Importer/GPUAssetImporter.h"
#include "Hazel/Renderer/ResourceHeapAllocator.h"
#include "Hazel/RHI/RHI.h"

#include <algorithm>
#include <thread>

namespace Hazel
{
    Renderer::Renderer(GraphicsContext* graphicsContext, Window* window)
        : m_GraphicsContext(graphicsContext),
          m_Instance(graphicsContext->GetInstance()),
          m_Device(graphicsContext->GetDevice()),
          m_Window(window),
          m_MaterialShaderRegistry(std::make_unique<MaterialShaderRegistry>(this)),
          m_BindlessRegistry(std::make_unique<BindlessRegistry>(this))
    {
#ifdef RHI_USE_VULKAN
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(
            m_Instance->GetHandle(),
            static_cast<GLFWwindow*>(m_Window->GetNativeWindow()),
            nullptr,
            &surface);
#endif
        m_WindowSurface = m_GraphicsContext->GetSurface();

        m_MaxFramesInFlight = GlobalSettings.Get(MaxFramesInFlightString, m_MaxFramesInFlight);
        m_SwapchainImageFormat = GlobalSettings.Get(SwapchainFormatString, m_SwapchainImageFormat);

        m_UsedGPUAssetsPerFrames = std::make_unique<UsedAssetPerFrames>(m_MaxFramesInFlight);

        CreateSwapchainResources();
        CreatePerFrameData();
        m_ResourceHeapAllocator = std::make_unique<ResourceHeapAllocator>(this);
        RecreateDefaultRenderTexture();
        CreateDefaultResources();
    }

    Renderer::~Renderer() = default;

    uint32_t Renderer::RegisterBindlessTexture(GPUAssetResolveResult texture)
    {
        return m_BindlessRegistry->RegisterTexture(std::move(texture));
    }

    uint32_t Renderer::RegisterBindlessSampler(GPUAssetResolveResult sampler)
    {
        return m_BindlessRegistry->RegisterSampler(std::move(sampler));
    }

    uint32_t Renderer::RegisterBindlessSamplerWithImage(GPUAssetResolveResult sampler, GPUAssetResolveResult image)
    {
        return m_BindlessRegistry->RegisterSamplerWithImage(std::move(sampler), std::move(image));
    }

    void Renderer::UnregisterBindlessTexture(uint32_t slot)
    {
        m_BindlessRegistry->UnregisterTexture(slot);
    }

    void Renderer::UnregisterBindlessSampler(uint32_t slot)
    {
        m_BindlessRegistry->UnregisterSampler(slot);
    }

    void Renderer::UnregisterBindlessSamplerWithImage(uint32_t slot)
    {
        m_BindlessRegistry->UnregisterCombinedImageSampler(slot);
    }

    GPUAssetResolveResult Renderer::ResolveGPUAsset(UUID uuid, AssetType type)
    {
        auto* assetManager = Project::GetActive()->GetAssetManager();
        auto* asset = assetManager->RequestAsset(uuid);

        if (!asset)
        {
            return GPUAssetResolveResult(GetDefaultGPUAsset(type), false);
        }

        auto currentAsset = m_GPUAssetRegistry.GetAsset(uuid);

        bool obsoleteMaterial = false;

        if (type == AssetType::Material)
        {
            if (auto* materialAsset = static_cast<CachedMaterial*>(currentAsset))
            {
                auto shader = ResolveGPUAsset(materialAsset->GetShader(), AssetType::Shader);
                if (!shader.asset)
                {
                    currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                    m_UsedGPUAssetsPerFrames->AddUsedAsset(GetCurrentFrameInFlightIndex(), currentAsset);
                    return GPUAssetResolveResult(currentAsset);
                }
                obsoleteMaterial = shader.asset->GetSourceVersion() > materialAsset->GetShaderSourceVersion();
            }
        }

        {
            auto& gpuState = asset->GetGPUAssetState();
            std::unique_lock lock(gpuState.mutex);
            if (gpuState.state == GPUAssetLoadState::Loaded && currentAsset &&
                gpuState.resolvedVersion >= asset->GetVersion() && !obsoleteMaterial)
            {
                currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                m_UsedGPUAssetsPerFrames->AddUsedAsset(GetCurrentFrameInFlightIndex(), currentAsset);
                return GPUAssetResolveResult(currentAsset);
            }

            if (gpuState.state == GPUAssetLoadState::Loading)
            {
                if (currentAsset)
                {
                    currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                    m_UsedGPUAssetsPerFrames->AddUsedAsset(GetCurrentFrameInFlightIndex(), currentAsset);
                    return GPUAssetResolveResult(currentAsset);
                }
                return GPUAssetResolveResult(GetDefaultGPUAsset(asset->GetType()));
            }

            gpuState.state = GPUAssetLoadState::Loading;
        }

        std::thread([this, asset, type] {
            ResolveGPUAssetWhileLoading(asset, type);
        }).detach();

        if (currentAsset)
        {
            currentAsset->SetLastReferencedFrame(m_CurrentFrame);
            m_UsedGPUAssetsPerFrames->AddUsedAsset(GetCurrentFrameInFlightIndex(), currentAsset);
            return GPUAssetResolveResult(currentAsset);
        }
        return GPUAssetResolveResult(GetDefaultGPUAsset(asset->GetType()));
    }

    GPUAssetResolveResult Renderer::ResolveGPUAssetBlocked(UUID uuid, AssetType type)
    {
        auto* assetManager = Project::GetActive()->GetAssetManager();
        auto* asset = assetManager->RequestAssetBlocked(uuid);

        if (!asset)
        {
            return GPUAssetResolveResult(GetDefaultGPUAsset(type), false);
        }

        {
            auto& gpuState = asset->GetGPUAssetState();
            std::unique_lock lock(gpuState.mutex);

            auto currentAsset = m_GPUAssetRegistry.GetAsset(uuid);

            bool obsoleteMaterial = false;

            if (type == AssetType::Material)
            {
                if (auto* materialAsset = static_cast<CachedMaterial*>(currentAsset))
                {
                    auto shader = ResolveGPUAssetBlocked(materialAsset->GetShader(), AssetType::Shader);
                    if (!shader.asset)
                    {
                        materialAsset->Return();
                        return GPUAssetResolveResult(nullptr, false);
                    }
                    obsoleteMaterial = shader.asset->GetSourceVersion() > materialAsset->GetShaderSourceVersion();
                }
            }

            if (gpuState.state == GPUAssetLoadState::Loaded && currentAsset &&
                gpuState.resolvedVersion >= asset->GetVersion() && !obsoleteMaterial)
            {
                currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                m_UsedGPUAssetsPerFrames->AddUsedAsset(GetCurrentFrameInFlightIndex(), currentAsset);
                return GPUAssetResolveResult(currentAsset);
            }

            if (currentAsset)
            {
                currentAsset->Return();
            }

            if (gpuState.state == GPUAssetLoadState::Loading)
            {
                gpuState.condition.wait(lock,
                                        [&gpuState] {
                                            return gpuState.state != GPUAssetLoadState::Loading;
                                        });

                currentAsset = m_GPUAssetRegistry.GetAsset(uuid);
                if (gpuState.state == GPUAssetLoadState::Loaded && currentAsset &&
                    gpuState.resolvedVersion >= asset->GetVersion())
                {
                    currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                    m_UsedGPUAssetsPerFrames->AddUsedAsset(GetCurrentFrameInFlightIndex(), currentAsset);
                    return GPUAssetResolveResult(currentAsset);
                }

                if (currentAsset)
                {
                    currentAsset->Return();
                }

                return GPUAssetResolveResult(GetDefaultGPUAsset(asset->GetType()), false);
            }

            gpuState.state = GPUAssetLoadState::Loading;
        }

        return ResolveGPUAssetWhileLoading(asset, type);
    }

    GPUAssetResolveResult Renderer::ResolveGPUGraphicsPipeline(
        UUID material,
        const std::vector<RHIFormat>& colorAttachmentFormats,
        RHIFormat depthStencilFormat)
    {
        return ResolveDirectGPUAsset<GPUGraphicsPipelineAsset>(
            material,
            colorAttachmentFormats,
            depthStencilFormat);
    }

    GPUAssetResolveResult Renderer::ResolveGPUGraphicsPipelineBlocked(
        UUID material,
        const std::vector<RHIFormat>& colorAttachmentFormats,
        RHIFormat depthStencilFormat)
    {
        return ResolveDirectGPUAssetBlocked<GPUGraphicsPipelineAsset>(
            material,
            colorAttachmentFormats,
            depthStencilFormat);
    }

    GPUAssetResolveResult Renderer::ResolveGPURenderTexture(const RenderTextureDesc& desc,
                                                            uint64_t lastReferencedFrame)
    {
        return ResolveDirectGPUAsset<GPURenderTextureAsset>(desc, lastReferencedFrame);
    }

    GPUAssetResolveResult Renderer::ResolveGPURenderBuffer(const RenderBufferDesc& desc,
                                                           uint64_t lastReferencedFrame)
    {
        return ResolveDirectGPUAsset<GPURenderBufferAsset>(desc, lastReferencedFrame);
    }

    GPUAssetResolveResult Renderer::ResolveGPUSampler(const RHISamplerDesc& desc,
                                                      uint64_t lastReferencedFrame)
    {
        return ResolveDirectGPUAsset<GPUSamplerAsset>(desc, lastReferencedFrame);
    }

    uint32_t Renderer::RegisterMaterial(UUID shader, uint64_t shaderSourceVersion, UUID material)
    {
        return m_MaterialShaderRegistry->RegisterMaterial(shader, shaderSourceVersion, material);
    }

    void Renderer::UnregisterMaterial(UUID shader, uint64_t shaderSourceVersion, uint32_t materialID)
    {
        m_MaterialShaderRegistry->UnregisterMaterial(shader, shaderSourceVersion, materialID);
    }

    void Renderer::RegisterShader(UUID uuid, uint64_t sourceVersion, const RHIShaderReflection& reflection)
    {
        m_MaterialShaderRegistry->RegisterShader(uuid, sourceVersion, reflection);
    }

    void Renderer::UnregisterShader(UUID uuid, uint64_t sourceVersion)
    {
        m_MaterialShaderRegistry->UnregisterShader(uuid, sourceVersion);
    }

    GPUAssetResolveResult Renderer::ResolveGPUAssetWhileLoading(Asset* asset, AssetType type)
    {
        auto& gpuState = asset->GetGPUAssetState();
        auto uuid = asset->GetUUID();

        auto newGPUAsset = LoadGPUAsset(asset);
        if (!newGPUAsset)
        {
            auto oldGPUAsset = m_GPUAssetRegistry.RemoveAsset(uuid);

            {
                std::unique_lock lock(gpuState.mutex);
                gpuState.resolvedVersion = 0;
                gpuState.state = GPUAssetLoadState::Unloaded;
            }

            m_GPUAssetRegistry.EnqueueForDeferredRelease(std::move(oldGPUAsset));
            gpuState.condition.notify_all();
            return GPUAssetResolveResult(GetDefaultGPUAsset(type), false);
        }

        auto newVersion = newGPUAsset->GetSourceVersion();
        auto [oldGPUAsset, currentGPUAsset] =
            m_GPUAssetRegistry.SetAssetAndGetTheOldAndTheNewOnes(std::move(newGPUAsset));

        {
            std::unique_lock lock(gpuState.mutex);
            gpuState.resolvedVersion = newVersion;
            gpuState.state = GPUAssetLoadState::Loaded;
        }

        m_GPUAssetRegistry.EnqueueForDeferredRelease(std::move(oldGPUAsset));
        gpuState.condition.notify_all();

        m_UsedGPUAssetsPerFrames->AddUsedAsset(GetCurrentFrameInFlightIndex(), currentGPUAsset);
        return GPUAssetResolveResult(currentGPUAsset);
    }

    void Renderer::CreateDefaultResources()
    {
        auto* cmd = m_GraphicsContext->AcquireDefaultCommandBuffer();
        cmd->Begin(true);

        // error texture
        uint8_t data[4] = {255, 255, 255, 255};
        RHIImageDesc imageDesc{};
        imageDesc.height = 1;
        imageDesc.width = 1;
        imageDesc.depth = 1;
        imageDesc.arrayLayers = 1;
        imageDesc.mipLevels = 1;
        imageDesc.format = RHIFormat::RGBA8UNorm;
        imageDesc.initialState = RHIImageResourceState::ShaderRead;
        imageDesc.usages = RHIImageUsageFlagBits::Sampled;
        auto image = RHIImage::Factory::CreateFromRawData(m_Device, cmd, imageDesc, data, 4);

        RHIImageViewDesc imageViewDesc{};
        imageViewDesc.format = RHIFormat::RGBA8UNorm;
        auto imageView = m_Device->CreateImageView(image, imageViewDesc);

        TextureDesc textureDesc{};
        textureDesc.format = RHIFormat::RGBA8UNorm;
        textureDesc.width = 1;
        textureDesc.height = 1;
        textureDesc.useMipmap = false;
        textureDesc.usages = RHIImageUsageFlagBits::Sampled;

        m_WhiteTexture = std::make_unique<GPUTextureAsset>(m_WhiteTextureUUID, 0, textureDesc, this, image, imageView);
        m_WhiteTextureBindingSlot = m_BindlessRegistry->RegisterTexture(
            GPUAssetResolveResult(m_WhiteTexture.get(), false));

        data[1] = 0;
        image = RHIImage::Factory::CreateFromRawData(m_Device, cmd, imageDesc, data, 4);
        imageView = m_Device->CreateImageView(image, imageViewDesc);
        m_ErrorTexture = std::make_unique<GPUTextureAsset>(m_ErrorTextureUUID, 0, textureDesc, this, image, imageView);
        m_ErrorTextureBindingSlot = m_BindlessRegistry->RegisterTexture(
            GPUAssetResolveResult(m_ErrorTexture.get(), false));

        RHISamplerDesc samplerDesc{};
        auto sampler = m_Device->CreateSampler(samplerDesc);
        m_DefaultSampler = std::make_unique<GPUSamplerAsset>(m_DefaultSamplerUUID,
                                                             0,
                                                             this,
                                                             samplerDesc,
                                                             sampler,
                                                             m_CurrentFrame);
        m_DefaultSamplerBindingSlot = m_BindlessRegistry->RegisterSampler(
            GPUAssetResolveResult(m_DefaultSampler.get(), false));

        m_WhiteTextureWithDefaultSamplerBindingSlot = m_BindlessRegistry->RegisterSamplerWithImage(
            GPUAssetResolveResult(m_WhiteTexture.get(), false),
            GPUAssetResolveResult(m_DefaultSampler.get(), false));

        cmd->End();

        RHIQueueSubmitDesc submitDesc{};
        submitDesc.commandBuffers = {cmd};
        RHISyncPoint syncPoint = m_Device->GetUniformQueue()->Submit(submitDesc);
        m_Device->WaitSyncPoint(&syncPoint);
        m_GraphicsContext->ReleaseDefaultCommandBuffer(cmd);
    }

    std::unique_ptr<GPUAsset> Renderer::LoadGPUAsset(Asset* asset)
    {
        if (asset)
        {
            asset->VersionUp();
        }
        switch (asset->GetType())
        {
            case AssetType::ComputeShader:
                return ImportGPUComputeShaderAsset(this, static_cast<ComputeShaderAsset*>(asset));
            case AssetType::RenderTexture:
                return ImportGPURenderTextureAsset(this, static_cast<RenderTextureAsset*>(asset));
            case AssetType::Sampler:
                return ImportGPUSamplerAsset(this, static_cast<SamplerAsset*>(asset));
            case AssetType::Shader:
                return ImportGPUShaderAsset(this, static_cast<ShaderAsset*>(asset));
            case AssetType::Texture:
                return ImportGPUTextureAsset(this, static_cast<TextureAsset*>(asset));
            case AssetType::Material:
                return ImportCachedMaterial(this, static_cast<MaterialAsset*>(asset));
            default:
                return nullptr;
        }
    }

    GPUAsset* Renderer::GetDefaultGPUAsset(AssetType type)
    {
        switch (type)
        {
            case AssetType::Texture:
                return m_WhiteTexture.get();
            case AssetType::Shader:
                return nullptr;
            case AssetType::Sampler:
                return m_DefaultSampler.get();
            case AssetType::RenderTexture:
                return m_DefaultRenderTexture.get();
            case AssetType::ComputeShader:
                return nullptr;
            case AssetType::Mesh:
                return nullptr;
            case AssetType::Material:
                return nullptr;
            default:
                return nullptr;
        }
    }

    void Renderer::CreatePerFrameData()
    {
        m_Frames.resize(m_MaxFramesInFlight);

        RHICommandPoolDesc commandPoolDesc{};
        commandPoolDesc.allowCommandBufferReset = true;
        commandPoolDesc.queueType = {};
        commandPoolDesc.transient = false;

        RHICommandBufferDesc commandBufferDesc{};
        commandBufferDesc.level = RHICommandBufferLevel::Primary;

        for (int i = 0; i < m_MaxFramesInFlight; i++)
        {
            m_Frames[i].commandPool = m_Device->CreateCommandPoolUniformQueue(commandPoolDesc);
            m_Frames[i].commandBuffer = m_Frames[i].commandPool->CreateCommandBuffer(commandBufferDesc);
        }
    }

    void Renderer::DestroyPerFrameData()
    {
        for (int i = 0; i < m_MaxFramesInFlight; i++)
        {
            if (m_Frames[i].commandPool)
            {
                m_Frames[i].commandPool->Release();
                m_Frames[i].commandPool = nullptr;
            }
        }
    }

    void Renderer::Render(Camera& camera)
    {
        auto& frameData = GetCurrentFrameData();
        auto* image = m_DefaultRenderTexture->GetImage();
        auto* imageView = m_DefaultRenderTexture->GetDefaultImageView();
        auto* cmd = frameData.commandBuffer;

        image->Transition(frameData.commandBuffer, image->GetCurrentState(), RHIImageResourceState::ColorAttachment);
        RHIRenderingAttachmentDesc colorAttachmentDesc{};
        colorAttachmentDesc.imageView = imageView;
        colorAttachmentDesc.loadOp = RHIRenderingLoadOp::Clear;
        colorAttachmentDesc.storeOp = RHIRenderingStoreOp::Store;
        colorAttachmentDesc.clearColorValue.float32 = {0.4f, 0.4f, 0.6f, 1.0f};
        colorAttachmentDesc.state = RHIImageResourceState::ColorAttachment;

        RHIRenderingInfo renderingInfo{};
        renderingInfo.colorAttachments = {colorAttachmentDesc};
        renderingInfo.renderOffset = {0, 0};
        renderingInfo.renderViewSize = {image->GetDesc().width, image->GetDesc().height};

        cmd->BeginRendering(renderingInfo);

        auto shader = ResolveGPUAsset(UUID(static_cast<uint64_t>(17413239457703166156)), AssetType::Shader);
        auto material = ResolveGPUAsset(UUID(static_cast<uint64_t>(6531589879330968622)), AssetType::Material);
        if (material.asset)
        {
            auto pipeline = ResolveGPUGraphicsPipeline(material.asset->GetUUID(),
                                                       {image->GetDesc().format},
                                                       RHIFormat::Undefined);
            if (pipeline.asset)
            {
                cmd->BindGraphicsPipeline(static_cast<GPUGraphicsPipelineAsset*>(pipeline.asset)->GetPipeline());

                cmd->SetViewport(0.0f,
                                 0.0f,
                                 static_cast<float>(image->GetDesc().width),
                                 static_cast<float>(image->GetDesc().height));
                cmd->SetScissor(0, 0, image->GetDesc().width, image->GetDesc().height);

                cmd->Draw(3, 1, 0, 0);
            }
        }

        cmd->EndRendering();
    }

    void Renderer::BeginSwapchainTargetRendering()
    {
        auto& frameData = GetCurrentFrameData();

        auto* swapchainImage = m_Swapchain->FetchImage(frameData.frameNumber);
        auto* swapchainImageView = m_Swapchain->FetchImageView(frameData.frameNumber);

        swapchainImage->Transition(
            frameData.commandBuffer,
            swapchainImage->GetCurrentState(),
            RHIImageResourceState::ColorAttachment);

        RHIRenderingAttachmentDesc colorAttachmentDesc{};
        colorAttachmentDesc.imageView = swapchainImageView;
        colorAttachmentDesc.loadOp = RHIRenderingLoadOp::Clear;
        colorAttachmentDesc.storeOp = RHIRenderingStoreOp::Store;
        colorAttachmentDesc.clearColorValue.float32 = {0.2f, 0.2f, 0.2f, 1.0f};
        colorAttachmentDesc.state = RHIImageResourceState::ColorAttachment;

        RHIRenderingInfo renderingInfo{};
        renderingInfo.colorAttachments = {colorAttachmentDesc};
        renderingInfo.renderOffset = {0, 0};
        renderingInfo.renderViewSize = {swapchainImage->GetDesc().width, swapchainImage->GetDesc().height};

        frameData.commandBuffer->BeginRendering(renderingInfo);
    }

    void Renderer::EndSwapchainTargetRendering()
    {
        auto& frameData = GetCurrentFrameData();
        frameData.commandBuffer->EndRendering();

        auto* swapchainImage = m_Swapchain->FetchImage(frameData.frameNumber);
        swapchainImage->Transition(
            frameData.commandBuffer,
            swapchainImage->GetCurrentState(),
            RHIImageResourceState::Present);
    }

    void Renderer::OnResize()
    {
        m_Device->WaitIdle();
        DestroySwapchainResources();
        CreateSwapchainResources();
    }

    void Renderer::OnViewportResize(uint32_t width, uint32_t height)
    {
        m_Device->WaitIdle();
        m_ViewportWidth = width;
        m_ViewportHeight = height;

        RecreateDefaultRenderTexture();
    }

    void Renderer::RecreateDefaultRenderTexture()
    {
        RenderTextureDesc renderTextureDesc{};
        renderTextureDesc.width = m_ViewportWidth;
        renderTextureDesc.height = m_ViewportHeight;
        renderTextureDesc.format = m_SwapchainImageFormat;
        renderTextureDesc.perFrame = true;
        renderTextureDesc.useMipmap = false;

        if (m_DefaultRenderTexture && m_DefaultRenderTexture->IsValid())
        {
            m_DefaultRenderTexture->ReleaseImmediate();
            m_DefaultRenderTexture.reset();
        }

        m_DefaultRenderTexture = CreateGPURenderTextureAsset(this,
                                                             m_DefaultRenderTextureUUID,
                                                             0,
                                                             renderTextureDesc,
                                                             m_CurrentFrame);
    }

    void Renderer::BeginFrame()
    {
        auto& frameData = GetCurrentFrameData();

        if (m_CurrentFrame >= m_MaxFramesInFlight)
            m_Device->WaitSyncPoint(&frameData.renderCompleteSyncPoint);
        frameData.commandBuffer->Reset();
        frameData.commandBuffer->Begin(true);

        auto result = m_Swapchain->AcquireImage();
        frameData.imageAvailableSyncPoint = result.availableSyncPoint;
        frameData.frameNumber = result.frameNumber;
    }

    void Renderer::EndFrame()
    {
        auto& frameData = GetCurrentFrameData();

        frameData.commandBuffer->End();

        RHIQueueSubmitDesc submitDesc{};
        submitDesc.waitSyncPoints = {frameData.imageAvailableSyncPoint};
        submitDesc.commandBuffers = {frameData.commandBuffer};
        frameData.renderCompleteSyncPoint = m_Device->GetUniformQueue()->Submit(submitDesc);

        m_Swapchain->SubmitFrame(frameData.frameNumber, {frameData.renderCompleteSyncPoint});

        auto usedAssets = m_UsedGPUAssetsPerFrames->GetUsedAssets(GetCurrentFrameInFlightIndex());

        for (auto* asset : usedAssets)
        {
            asset->SetLastReferencedInfo(m_CurrentFrame, frameData.renderCompleteSyncPoint);
        }

        auto garbage = m_GPUAssetRegistry.AcquireSomeAssetsForGarbageCollection(
            m_CurrentFrame - kDefaultGPUAssetGarbageCollectFrameThreshold);
        for (auto& asset : garbage)
        {
            m_GPUAssetRegistry.EnqueueForDeferredRelease(std::move(asset));
        }

        m_GPUAssetRegistry.FlushGPUAssetReleaseQueue();

        Step();
    }

    void Renderer::CreateSwapchainResources()
    {
        RHISwapchainDesc swapchainDesc{};
        swapchainDesc.height = m_Window->GetHeight();
        swapchainDesc.width = m_Window->GetWidth();
        swapchainDesc.format = m_SwapchainImageFormat;
        swapchainDesc.imageCount = m_MaxFramesInFlight;
        swapchainDesc.mode = RHISwapchainMode::Mailbox;
        swapchainDesc.usages = RHIImageUsageFlagBits::ColorAttachment | RHIImageUsageFlagBits::TransferDestination;
        swapchainDesc.surface = m_WindowSurface;
        m_Swapchain = m_Device->CreateSwapchain(swapchainDesc);
    }

    void Renderer::DestroySwapchainResources()
    {
        if (m_Swapchain)
        {
            m_Swapchain->ReleaseImmediate();
            m_Swapchain = nullptr;
        }
    }

    void Renderer::Release()
    {
        m_MaterialShaderRegistry.reset();

        if (m_ResourceHeapAllocator)
        {
            m_ResourceHeapAllocator->Release();
            m_ResourceHeapAllocator.reset();
        }
    }
} // namespace Hazel