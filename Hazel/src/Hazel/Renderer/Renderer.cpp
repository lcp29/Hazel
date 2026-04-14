#include "Hazel/Renderer/Renderer.h"

#include "GPUAsset/GPUGraphicsPipelineAsset.h"
#include "Hazel/Asset/ComputeShaderAsset.h"
#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Asset/MeshAsset.h"
#include "Hazel/Asset/RenderTextureAsset.h"
#include "Hazel/Asset/SamplerAsset.h"
#include "Hazel/Asset/ShaderAsset.h"
#include "Hazel/Asset/TextureAsset.h"
#include "Hazel/Project/GlobalSettingRegistry.h"
#include "Hazel/Project/Project.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/GPUAsset/CachedMaterial.h"
#include "Hazel/Renderer/GPUAsset/GPUComputeShaderAsset.h"
#include "Hazel/Renderer/GPUAsset/GPURenderTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUSamplerAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUShaderAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/Importer/GPUAssetImporter.h"
#include "Hazel/Renderer/ResourceHeapAllocator.h"
#include "Hazel/Scene/Scene.h"
#include "glm/gtx/iteration.hpp"

#include <algorithm>
#include <array>
#include <thread>

namespace Hazel
{
    Renderer::Renderer(GraphicsContext* graphicsContext, Window* window)
        : m_GraphicsContext(graphicsContext)
        , m_Instance(graphicsContext->GetInstance())
        , m_Device(graphicsContext->GetDevice())
        , m_Window(window)
    {
		// surface and swapchain init
#ifdef RHI_USE_VULKAN
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(
            m_Instance->GetHandle(), static_cast<GLFWwindow*>(m_Window->GetNativeWindow()), nullptr, &surface);
#endif
        m_WindowSurface = m_GraphicsContext->GetSurface();
        m_MaxFramesInFlight = GlobalSettings.Get(MaxFramesInFlightString, m_MaxFramesInFlight);
        m_SwapchainImageFormat = GlobalSettings.Get(SwapchainFormatString, m_SwapchainImageFormat);
        CreateSwapchainResources();

        m_GPUAssetGarbageCollectFrameThreshold =
            GlobalSettings.Get(GPUAssetGarbageCollectFrameThresholdString, m_GPUAssetGarbageCollectFrameThreshold);

        m_ResourceHeapAllocator = std::make_unique<ResourceHeapAllocator>(this);

		// assets
        m_GPUAssetRegistry = std::make_unique<GPUAssetRegistry>();

        m_ResourceBindingRegistry = std::make_unique<ResourceBindingRegistry>(this);
        m_ResourceBindingRegistry->CreatePerViewResources();

        CreatePerFrameData();

        RecreateDefaultRenderTexture();
        CreateDefaultResources();

		// render scene
        m_RenderScene = std::make_unique<RenderScene>(this);
        m_GeometryDataRegistry = std::make_unique<GeometryDataRegistry>(this);
    }

    uint32_t Renderer::RegisterBindlessTexture(GPUAssetHandle texture)
    {
        return GetResourceBindingRegistry()->RegisterTexture(std::move(texture));
    }

    uint32_t Renderer::RegisterBindlessSampler(GPUAssetHandle sampler)
    {
        return GetResourceBindingRegistry()->RegisterSampler(std::move(sampler));
    }

    uint32_t Renderer::RegisterBindlessSamplerWithImage(GPUAssetHandle sampler, GPUAssetHandle image)
    {
        return GetResourceBindingRegistry()->RegisterSamplerWithImage(std::move(sampler), std::move(image));
    }

    void Renderer::UnregisterBindlessTexture(uint32_t slot) { GetResourceBindingRegistry()->UnregisterTexture(slot); }

    void Renderer::UnregisterBindlessSampler(uint32_t slot) { GetResourceBindingRegistry()->UnregisterSampler(slot); }

    void Renderer::UnregisterBindlessSamplerWithImage(uint32_t slot)
    {
        GetResourceBindingRegistry()->UnregisterCombinedImageSampler(slot);
    }

    GPUAssetHandle Renderer::ResolveGPUAsset(UUID uuid, AssetType type)
    {
        auto* assetManager = Project::GetActive()->GetAssetManager();
        auto* asset = assetManager->RequestAsset(uuid);

        if (!asset) { return {GetDefaultGPUAsset(type), false}; }

        auto currentAsset = m_GPUAssetRegistry->GetAsset(uuid);

        bool shaderObsolete = false;

        if (type == AssetType::Material)
        {
            if (auto* materialAsset = static_cast<CachedMaterial*>(currentAsset))
            {
                auto shader = ResolveGPUAsset(materialAsset->GetShader(), AssetType::Shader);
                if (!shader.asset)
                {
                    currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                    return {currentAsset};
                }
                shaderObsolete = shader.asset->GetSourceVersion() > materialAsset->GetShaderSourceVersion();
            }
        }

        {
            auto& gpuState = asset->GetGPUAssetState();
            std::unique_lock lock(gpuState.mutex);
            if (gpuState.state == GPUAssetLoadState::Loaded && currentAsset
                && gpuState.resolvedVersion >= asset->GetVersion() && !shaderObsolete)
            {
                currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                return {currentAsset};
            }

            if (gpuState.state == GPUAssetLoadState::Loading)
            {
                if (currentAsset && !(type == AssetType::Material && shaderObsolete))
                {
                    currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                    return {currentAsset};
                }
                return {GetDefaultGPUAsset(asset->GetType())};
            }

            gpuState.state = GPUAssetLoadState::Loading;
        }

        std::thread([this, asset, type] { ResolveGPUAssetWhileLoading(asset, type); }).detach();

        if (currentAsset)
        {
            currentAsset->SetLastReferencedFrame(m_CurrentFrame);
            return {currentAsset};
        }
        return {GetDefaultGPUAsset(asset->GetType())};
    }

    GPUAssetHandle Renderer::ResolveGPUAssetBlocked(UUID uuid, AssetType type)
    {
        auto* assetManager = Project::GetActive()->GetAssetManager();
        auto* asset = assetManager->RequestAssetBlocked(uuid);

        if (!asset) { return {GetDefaultGPUAsset(type), false}; }

        {
            auto& gpuState = asset->GetGPUAssetState();
            std::unique_lock lock(gpuState.mutex);

            auto currentAsset = m_GPUAssetRegistry->GetAsset(uuid);

            bool shaderObsolete = false;

            if (type == AssetType::Material)
            {
                if (auto* materialAsset = static_cast<CachedMaterial*>(currentAsset))
                {
                    auto shader = ResolveGPUAssetBlocked(materialAsset->GetShader(), AssetType::Shader);
                    if (!shader.asset)
                    {
                        materialAsset->Return();
                        return {nullptr, false};
                    }
                    shaderObsolete = shader.asset->GetSourceVersion() > materialAsset->GetShaderSourceVersion();
                }
            }

            if (gpuState.state == GPUAssetLoadState::Loaded && currentAsset
                && gpuState.resolvedVersion >= asset->GetVersion() && !shaderObsolete)
            {
                currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                return {currentAsset};
            }

            if (currentAsset) { currentAsset->Return(); }

            if (gpuState.state == GPUAssetLoadState::Loading)
            {
                gpuState.condition.wait(lock, [&gpuState] { return gpuState.state != GPUAssetLoadState::Loading; });

                currentAsset = m_GPUAssetRegistry->GetAsset(uuid);
                if (gpuState.state == GPUAssetLoadState::Loaded && currentAsset
                    && gpuState.resolvedVersion >= asset->GetVersion())
                {
                    if (type == AssetType::Material)
                    {
                        auto* materialAsset = static_cast<CachedMaterial*>(currentAsset);
                        auto shader = ResolveGPUAssetBlocked(materialAsset->GetShader(), AssetType::Shader);
                        if (!shader.asset)
                        {
                            currentAsset->Return();
                            return {GetDefaultGPUAsset(asset->GetType()), false};
                        }

                        const bool loadedShaderObsolete =
                            shader.asset->GetSourceVersion() > materialAsset->GetShaderSourceVersion();
                        if (loadedShaderObsolete)
                        {
                            currentAsset->Return();
                            return {GetDefaultGPUAsset(asset->GetType()), false};
                        }
                    }

                    currentAsset->SetLastReferencedFrame(m_CurrentFrame);
                    return {currentAsset};
                }

                if (currentAsset) { currentAsset->Return(); }

                return {GetDefaultGPUAsset(asset->GetType()), false};
            }

            gpuState.state = GPUAssetLoadState::Loading;
        }

        return ResolveGPUAssetWhileLoading(asset, type);
    }

    GPUAssetHandle
    Renderer::ResolveGPUGraphicsPipeline(UUID material,
                                         const std::vector<RHIFormat>& colorAttachmentFormats,
                                         const std::vector<RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                                         RHIFormat depthStencilFormat)
    {
        return ResolveDirectGPUAsset<GPUGraphicsPipelineAsset>(
            material, colorAttachmentFormats, colorBlendAttachments, depthStencilFormat);
    }

    GPUAssetHandle
    Renderer::ResolveGPUGraphicsPipelineBlocked(UUID material,
                                                const std::vector<RHIFormat>& colorAttachmentFormats,
                                                const std::vector<RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                                                RHIFormat depthStencilFormat)
    {
        return ResolveDirectGPUAssetBlocked<GPUGraphicsPipelineAsset>(
            material, colorAttachmentFormats, colorBlendAttachments, depthStencilFormat);
    }

    GPUAssetHandle Renderer::ResolveGPURenderTexture(const RenderTextureDesc& desc, uint64_t lastReferencedFrame)
    {
        return ResolveDirectGPUAsset<GPURenderTextureAsset>(desc, lastReferencedFrame);
    }

    GPUAssetHandle Renderer::ResolveGPURenderBuffer(const RenderBufferDesc& desc, uint64_t lastReferencedFrame)
    {
        return ResolveDirectGPUAsset<GPURenderBufferAsset>(desc, lastReferencedFrame);
    }

    GPUAssetHandle Renderer::ResolveGPUSampler(const RHISamplerDesc& desc, uint64_t lastReferencedFrame)
    {
        return ResolveDirectGPUAsset<GPUSamplerAsset>(desc, lastReferencedFrame);
    }

    uint32_t Renderer::RegisterMaterial(UUID shader, uint64_t shaderSourceVersion, UUID material)
    {
        return GetResourceBindingRegistry()->RegisterMaterial(shader, shaderSourceVersion, material);
    }

    void Renderer::UnregisterMaterial(UUID shader, uint64_t shaderSourceVersion, uint32_t materialID)
    {
        GetResourceBindingRegistry()->UnregisterMaterial(shader, shaderSourceVersion, materialID);
    }

    void Renderer::RegisterShader(UUID uuid, uint64_t sourceVersion, const RHIShaderReflection& reflection)
    {
        GetResourceBindingRegistry()->RegisterShader(uuid, sourceVersion, reflection);
    }

    void Renderer::UnregisterShader(UUID uuid, uint64_t sourceVersion)
    {
        GetResourceBindingRegistry()->UnregisterShader(uuid, sourceVersion);
    }

    GPUAssetHandle Renderer::ResolveGPUAssetWhileLoading(Asset* asset, AssetType type)
    {
        auto& gpuState = asset->GetGPUAssetState();
        auto uuid = asset->GetUUID();

        auto newGPUAsset = LoadGPUAsset(asset);
        if (!newGPUAsset)
        {
            auto oldGPUAsset = m_GPUAssetRegistry->RemoveAsset(uuid);

            {
                std::unique_lock lock(gpuState.mutex);
                gpuState.resolvedVersion = 0;
                gpuState.state = GPUAssetLoadState::Unloaded;
            }

            m_GPUAssetRegistry->EnqueueForDeferredRelease(std::move(oldGPUAsset));
            gpuState.condition.notify_all();
            return {GetDefaultGPUAsset(type), false};
        }

        auto newVersion = newGPUAsset->GetSourceVersion();
        auto [oldGPUAsset, currentGPUAsset] =
            m_GPUAssetRegistry->SetAssetAndGetTheOldAndTheNewOnes(std::move(newGPUAsset));

        {
            std::unique_lock lock(gpuState.mutex);
            gpuState.resolvedVersion = newVersion;
            gpuState.state = GPUAssetLoadState::Loaded;
        }

        m_GPUAssetRegistry->EnqueueForDeferredRelease(std::move(oldGPUAsset));
        gpuState.condition.notify_all();

        return {currentGPUAsset};
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
        m_WhiteTextureBindingSlot =
            GetResourceBindingRegistry()->RegisterTexture(GPUAssetHandle(m_WhiteTexture.get(), false));

        data[1] = 0;
        image = RHIImage::Factory::CreateFromRawData(m_Device, cmd, imageDesc, data, 4);
        imageView = m_Device->CreateImageView(image, imageViewDesc);
        m_ErrorTexture = std::make_unique<GPUTextureAsset>(m_ErrorTextureUUID, 0, textureDesc, this, image, imageView);
        m_ErrorTextureBindingSlot =
            GetResourceBindingRegistry()->RegisterTexture(GPUAssetHandle(m_ErrorTexture.get(), false));

        RHISamplerDesc samplerDesc{};
        auto sampler = m_Device->CreateSampler(samplerDesc);
        m_DefaultSampler =
            std::make_unique<GPUSamplerAsset>(m_DefaultSamplerUUID, 0, this, samplerDesc, sampler, m_CurrentFrame);
        m_DefaultSamplerBindingSlot =
            GetResourceBindingRegistry()->RegisterSampler(GPUAssetHandle(m_DefaultSampler.get(), false));

        m_WhiteTextureWithDefaultSamplerBindingSlot = GetResourceBindingRegistry()->RegisterSamplerWithImage(
            GPUAssetHandle(m_WhiteTexture.get(), false), GPUAssetHandle(m_DefaultSampler.get(), false));

        cmd->End();

        RHIQueueSubmitDesc submitDesc{};
        submitDesc.commandBuffers = {cmd};
        RHISyncPoint syncPoint = m_Device->GetUniformQueue()->Submit(submitDesc);
        m_Device->WaitSyncPoint(&syncPoint);
        m_GraphicsContext->ReleaseDefaultCommandBuffer(cmd);
    }

    std::unique_ptr<GPUAsset> Renderer::LoadGPUAsset(Asset* asset)
    {
        if (!asset) { return nullptr; }
        asset->VersionUp();
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
            // TODO: TEMP URGENT INTERVIEW: mesh asset import
            case AssetType::Mesh:
                return ImportGPUMeshAsset(this, static_cast<MeshAsset*>(asset));
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
                return nullptr;
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

        RHICommandPoolDesc commandPoolDesc{.queueType = {}, .transient = false, .allowCommandBufferReset = true};

        RHICommandBufferDesc commandBufferDesc{.level = RHICommandBufferLevel::Primary};

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

    void Renderer::Render()
    {
        auto& frameData = GetCurrentFrameData();
        auto* cmd = frameData.commandBuffer;

        GetResourceBindingRegistry()->SetValue<float>("lightStrength", 100.0);

        RHIRenderingAttachmentDesc colorAttachmentDesc{};
        colorAttachmentDesc.imageView = m_DefaultRenderTexture->GetDefaultImageView();
        colorAttachmentDesc.loadOp = RHIRenderingLoadOp::Clear;
        colorAttachmentDesc.storeOp = RHIRenderingStoreOp::Store;
        colorAttachmentDesc.clearColorValue.float32 = {0.0f, 0.0f, 0.0f, 1.0f};
        colorAttachmentDesc.state = RHIImageResourceState::ColorAttachment;

        RHIRenderingAttachmentDesc depthStencilAttachmentDesc{};
        depthStencilAttachmentDesc.imageView = m_DefaultDepthRenderTexture->GetDefaultImageView();
        depthStencilAttachmentDesc.loadOp = RHIRenderingLoadOp::Clear;
        depthStencilAttachmentDesc.storeOp = RHIRenderingStoreOp::DontCare;
        depthStencilAttachmentDesc.clearDepthStencilValue.depth = 1.0f;
        depthStencilAttachmentDesc.clearDepthStencilValue.stencil = 0;
        depthStencilAttachmentDesc.state = RHIImageResourceState::DepthStencilAttachment;

        m_DefaultRenderTexture->GetImage()->Transition(
            cmd, m_DefaultRenderTexture->GetImage()->GetCurrentState(), RHIImageResourceState::ColorAttachment);
        m_DefaultDepthRenderTexture->GetImage()->Transition(cmd,
                                                            m_DefaultDepthRenderTexture->GetImage()->GetCurrentState(),
                                                            RHIImageResourceState::DepthStencilAttachment);

        RHIRect2D viewportArea = {{0, 0}, {m_ViewportWidth, m_ViewportHeight}};

        auto testImage = ResolveGPUAsset(10859876451405640399ull, AssetType::Texture);

        GetResourceBindingRegistry()->SetImage("testImage", &testImage);

        RunGraphicsPass(cmd,
                        &m_Cameras[0],
                        {colorAttachmentDesc},
                        {RHIColorBlendAttachmentDesc{
                            .blendEnable = true,
                            .srcColorBlendFactor = RHIBlendFactor::SrcAlpha,
                            .dstColorBlendFactor = RHIBlendFactor::OneMinusSrcAlpha,
                            .colorBlendOp = RHIBlendOp::Add,
                            .srcAlphaBlendFactor = RHIBlendFactor::One,
                            .dstAlphaBlendFactor = RHIBlendFactor::Zero,
                            .alphaBlendOp = RHIBlendOp::Add,
                        }},
                        &depthStencilAttachmentDesc,
                        viewportArea,
                        viewportArea,
                        viewportArea);
    }

    void Renderer::BeginSwapchainTargetRendering()
    {
        auto& frameData = GetCurrentFrameData();

        auto* swapchainImage = m_Swapchain->FetchImage(frameData.frameNumber);
        auto* swapchainImageView = m_Swapchain->FetchImageView(frameData.frameNumber);

        swapchainImage->Transition(
            frameData.commandBuffer, swapchainImage->GetCurrentState(), RHIImageResourceState::ColorAttachment);

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
            frameData.commandBuffer, swapchainImage->GetCurrentState(), RHIImageResourceState::Present);
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
        if (m_DefaultRenderTexture && m_DefaultRenderTexture->IsValid())
        {
            m_DefaultRenderTexture->ReleaseImmediate();
            m_DefaultRenderTexture.reset();
        }

        if (m_DefaultDepthRenderTexture && m_DefaultDepthRenderTexture->IsValid())
        {
            m_DefaultDepthRenderTexture->ReleaseImmediate();
            m_DefaultDepthRenderTexture.reset();
        }

        RenderTextureDesc renderTextureDesc{};
        renderTextureDesc.width = m_ViewportWidth;
        renderTextureDesc.height = m_ViewportHeight;
        renderTextureDesc.format = m_SwapchainImageFormat;
        renderTextureDesc.perFrame = true;
        renderTextureDesc.useMipmap = false;

        m_DefaultRenderTexture =
            CreateGPURenderTextureAsset(this, m_DefaultRenderTextureUUID, 0, renderTextureDesc, m_CurrentFrame);

        renderTextureDesc.format = RHIFormat::D32SFloatS8Uint;

        m_DefaultDepthRenderTexture = CreateGPURenderTextureAsset(this, UUID(), 0, renderTextureDesc, m_CurrentFrame);
    }

    void Renderer::BeginFrame()
    {
        auto& frameData = GetCurrentFrameData();

        if (m_CurrentFrame >= m_MaxFramesInFlight) { m_Device->WaitSyncPoint(&frameData.renderCompleteSyncPoint); }

        frameData.commandBuffer->Reset();
        frameData.commandBuffer->Begin(true);

        auto result = m_Swapchain->AcquireImage(1000000000000ull);
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

        auto garbage = m_GPUAssetRegistry->AcquireSomeAssetsForGarbageCollection(
            m_CurrentFrame - m_GPUAssetGarbageCollectFrameThreshold);
        for (auto& asset : garbage)
        {
            m_GPUAssetRegistry->EnqueueForDeferredRelease(std::move(asset));
        }

        m_GPUAssetRegistry->FlushGPUAssetReleaseQueue();

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

    // TODO: TEMP URGENT INTERVIEW: a vanilla forward pass
    void Renderer::RunGraphicsPass(RHICommandBuffer* cmd,
                                   SceneCameraView* camera,
                                   const std::vector<RHIRenderingAttachmentDesc>& colorAttachmentDescriptions,
                                   const std::vector<RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                                   const RHIRenderingAttachmentDesc* depthStencilAttachmentDescription,
                                   RHIRect2D renderArea,
                                   RHIRect2D viewportArea,
                                   RHIRect2D scissorArea)
    {
        const glm::mat4 view = camera->transform.GetView();
        const glm::mat4 projection = camera->camera->GetProjection();
        const glm::mat4 viewProjection = projection * view;
        GetResourceBindingRegistry()->SetViewProjectionMatrix(view, projection);

        const glm::vec4 row0(viewProjection[0][0], viewProjection[1][0], viewProjection[2][0], viewProjection[3][0]);
        const glm::vec4 row1(viewProjection[0][1], viewProjection[1][1], viewProjection[2][1], viewProjection[3][1]);
        const glm::vec4 row2(viewProjection[0][2], viewProjection[1][2], viewProjection[2][2], viewProjection[3][2]);
        const glm::vec4 row3(viewProjection[0][3], viewProjection[1][3], viewProjection[2][3], viewProjection[3][3]);

        std::array<glm::vec4, 6> worldFrustumPlanes = {
            row3 + row0,
            row3 - row0,
            row3 + row1,
            row3 - row1,
            row3 + row2,
            row3 - row2,
        };
        for (auto& plane : worldFrustumPlanes)
        {
            const float normalLength = glm::length(glm::vec3(plane));
            if (normalLength > 0.0f) { plane /= normalLength; }
        }

        std::vector<RHIFormat> colorAttachmentFormats;
        auto depthStencilFormat = RHIFormat::Undefined;

        RHIRenderingInfo renderingInfo{};
        renderingInfo.renderOffset = renderArea.offset;
        renderingInfo.renderViewSize = renderArea.extent;

        for (const auto& colorDescription : colorAttachmentDescriptions)
        {
            renderingInfo.colorAttachments.push_back(colorDescription);
            colorAttachmentFormats.push_back(colorDescription.imageView->GetFormat());
        }

        if (depthStencilAttachmentDescription)
        {
            renderingInfo.depthAttachment = *depthStencilAttachmentDescription;
            renderingInfo.stencilAttachment = *depthStencilAttachmentDescription;
            depthStencilFormat = depthStencilAttachmentDescription->imageView->GetFormat();
        }

        bool firstBind = true;

        cmd->BeginRendering(renderingInfo);

        const auto& renderObjects = GetRenderScene()->GetRenderObjectsSortedByShader();

        for (auto it = renderObjects.begin(); it != renderObjects.end();)
        {
            auto [begin, end] = renderObjects.equal_range(it->first);
            if (it->first == UUID(-1))
            {
                it = end;
                continue;
            }

            auto shaderResult = ResolveGPUAsset(it->first, AssetType::Shader);
            if (!shaderResult.asset)
            {
                it = end;
                continue;
            }
            auto* shader = static_cast<GPUShaderAsset*>(shaderResult.asset);

            GetResourceBindingRegistry()->UpdateUserUploadDataForShader(shader->GetUUID(), shader->GetSourceVersion());
            GetResourceBindingRegistry()->CreateOrUpdatePerShaderResourcesForShader(shader->GetUUID(),
                                                                                    shader->GetSourceVersion());

            if (firstBind)
            {
                GetResourceBindingRegistry()->BindPerViewResources(cmd, shader->GetUUID(), shader->GetSourceVersion());
                firstBind = false;
            }

            GetResourceBindingRegistry()->BindUserUploadResources(cmd, shader->GetUUID(), shader->GetSourceVersion());
            GetResourceBindingRegistry()->BindMaterialPropertyResources(
                cmd, shader->GetUUID(), shader->GetSourceVersion());

            for (; it != end; ++it)
            {
                auto* renderObject = it->second;

                auto meshResult = ResolveGPUAsset(renderObject->mesh, AssetType::Mesh);
                if (!meshResult.asset) { continue; }
                auto* mesh = static_cast<GPUMeshAsset*>(meshResult.asset);

                bool shouldCull = false;
                const glm::mat4 worldToObjectPlaneTransform = glm::transpose(renderObject->transform);
                for (const auto& worldPlane : worldFrustumPlanes)
                {
                    glm::vec4 objectPlane = worldToObjectPlaneTransform * worldPlane;
                    const float normalLength = glm::length(glm::vec3(objectPlane));
                    if (normalLength <= 0.0f) { continue; }
                    objectPlane /= normalLength;

                    const float distance =
                        glm::dot(glm::vec3(objectPlane), mesh->GetBoundingSphereCenter()) + objectPlane.w;
                    if (distance < -mesh->GetBoundingSphereRadius())
                    {
                        shouldCull = true;
                        break;
                    }
                }
                if (shouldCull) { continue; }

                auto materialResult = ResolveGPUAsset(renderObject->material, AssetType::Material);
                if (!materialResult.asset) { continue; }
                auto* material = static_cast<CachedMaterial*>(materialResult.asset);

                auto pipelineResult = ResolveGPUGraphicsPipeline(
                    renderObject->material, colorAttachmentFormats, colorBlendAttachments, depthStencilFormat);
                if (!pipelineResult.asset) { continue; }
                auto* pipeline = static_cast<GPUGraphicsPipelineAsset*>(pipelineResult.asset);

                cmd->BindGraphicsPipeline(pipeline->GetPipeline());
                cmd->BindVertexBuffer(0, mesh->GetVertexBuffer(), 0);
                cmd->BindIndexBuffer(mesh->GetIndexBuffer(), RHIIndexType::UInt32, 0);

                auto materialID = material->GetMaterialID();

                cmd->PushConstants(GetResourceBindingRegistry()->GetShaderResourceSignature(shader->GetUUID(),
                                                                                            shader->GetSourceVersion()),
                                   RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,
                                   0,
                                   64,
                                   &renderObject->transform);

                cmd->PushConstants(GetResourceBindingRegistry()->GetShaderResourceSignature(shader->GetUUID(),
                                                                                            shader->GetSourceVersion()),
                                   RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,
                                   64,
                                   4,
                                   &materialID);

                uint32_t entityID = renderObject->enttEntity;

                cmd->PushConstants(GetResourceBindingRegistry()->GetShaderResourceSignature(shader->GetUUID(),
                                                                                            shader->GetSourceVersion()),
                                   RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,
                                   68,
                                   4,
                                   &entityID);

                cmd->SetViewport(viewportArea.offset.x,
                                 viewportArea.offset.y,
                                 viewportArea.extent.width,
                                 viewportArea.extent.height);
                cmd->SetScissor(
                    scissorArea.offset.x, scissorArea.offset.y, scissorArea.extent.width, scissorArea.extent.height);
                cmd->SetBlendConstants(0.0f, 0.0f, 0.0f, 0.0f);

                cmd->DrawIndexed(mesh->GetIndices().size(), 1, 0, 0, 0);
            }
        }

        cmd->EndRendering();
    }

    void Renderer::RunGraphicsPass(RHICommandBuffer* cmd,
                                   UUID overrideMaterial,
                                   SceneCameraView* camera,
                                   const std::vector<RHIRenderingAttachmentDesc>& colorAttachmentDescriptions,
                                   const std::vector<RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                                   const RHIRenderingAttachmentDesc* depthStencilAttachmentDescription,
                                   RHIRect2D renderArea,
                                   RHIRect2D viewportArea,
                                   RHIRect2D scissorArea)
    {
        const glm::mat4 view = camera->transform.GetView();
        const glm::mat4 projection = camera->camera->GetProjection();
        const glm::mat4 viewProjection = projection * view;
        GetResourceBindingRegistry()->SetViewProjectionMatrix(view, projection);

        const glm::vec4 row0(viewProjection[0][0], viewProjection[1][0], viewProjection[2][0], viewProjection[3][0]);
        const glm::vec4 row1(viewProjection[0][1], viewProjection[1][1], viewProjection[2][1], viewProjection[3][1]);
        const glm::vec4 row2(viewProjection[0][2], viewProjection[1][2], viewProjection[2][2], viewProjection[3][2]);
        const glm::vec4 row3(viewProjection[0][3], viewProjection[1][3], viewProjection[2][3], viewProjection[3][3]);

        std::array<glm::vec4, 6> worldFrustumPlanes = {
            row3 + row0,
            row3 - row0,
            row3 + row1,
            row3 - row1,
            row3 + row2,
            row3 - row2,
        };
        for (auto& plane : worldFrustumPlanes)
        {
            const float normalLength = glm::length(glm::vec3(plane));
            if (normalLength > 0.0f) { plane /= normalLength; }
        }

        std::vector<RHIFormat> colorAttachmentFormats;
        auto depthStencilFormat = RHIFormat::Undefined;

        auto materialResult = ResolveGPUAsset(overrideMaterial, AssetType::Material);
        if (!materialResult.asset) { return; }
        auto shaderResult =
            ResolveGPUAsset(static_cast<CachedMaterial*>(materialResult.asset)->GetShader(), AssetType::Shader);

        auto* material = static_cast<CachedMaterial*>(materialResult.asset);
        auto* shader = static_cast<GPUShaderAsset*>(shaderResult.asset);

        RHIRenderingInfo renderingInfo{};
        renderingInfo.renderOffset = renderArea.offset;
        renderingInfo.renderViewSize = renderArea.extent;

        for (const auto& colorDescription : colorAttachmentDescriptions)
        {
            renderingInfo.colorAttachments.push_back(colorDescription);
            colorAttachmentFormats.push_back(colorDescription.imageView->GetFormat());
        }

        if (depthStencilAttachmentDescription)
        {
            renderingInfo.depthAttachment = *depthStencilAttachmentDescription;
            renderingInfo.stencilAttachment = *depthStencilAttachmentDescription;
            depthStencilFormat = depthStencilAttachmentDescription->imageView->GetFormat();
        }

        if (!shaderResult.asset) { return; }
        auto pipelineResult = ResolveGPUGraphicsPipeline(
            overrideMaterial, colorAttachmentFormats, colorBlendAttachments, depthStencilFormat);
        if (!pipelineResult.asset) { return; }
        auto* pipeline = static_cast<GPUGraphicsPipelineAsset*>(pipelineResult.asset);

        cmd->BeginRendering(renderingInfo);

        cmd->BindGraphicsPipeline(pipeline->GetPipeline());

        GetResourceBindingRegistry()->UpdateUserUploadDataForShader(shader->GetUUID(), shader->GetSourceVersion());
        GetResourceBindingRegistry()->BindPerViewResources(cmd, shader->GetUUID(), shader->GetSourceVersion());
        GetResourceBindingRegistry()->BindUserUploadResources(cmd, shader->GetUUID(), shader->GetSourceVersion());
        GetResourceBindingRegistry()->BindMaterialPropertyResources(cmd, shader->GetUUID(), shader->GetSourceVersion());

        const auto& renderObjects = GetRenderScene()->GetRenderObjectsSortedByShader();

        for (auto* renderObject : renderObjects | std::views::values)
        {
            auto meshResult = ResolveGPUAsset(renderObject->mesh, AssetType::Mesh);
            if (!meshResult.asset) { continue; }
            auto* mesh = static_cast<GPUMeshAsset*>(meshResult.asset);

            bool shouldCull = false;
            const glm::mat4 worldToObjectPlaneTransform = glm::transpose(renderObject->transform);
            for (const auto& worldPlane : worldFrustumPlanes)
            {
                glm::vec4 objectPlane = worldToObjectPlaneTransform * worldPlane;
                const float normalLength = glm::length(glm::vec3(objectPlane));
                if (normalLength <= 0.0f) { continue; }
                objectPlane /= normalLength;

                const float distance =
                    glm::dot(glm::vec3(objectPlane), mesh->GetBoundingSphereCenter()) + objectPlane.w;
                if (distance < -mesh->GetBoundingSphereRadius())
                {
                    shouldCull = true;
                    break;
                }
            }
            if (shouldCull) { continue; }

            cmd->BindVertexBuffer(0, mesh->GetVertexBuffer(), 0);
            cmd->BindIndexBuffer(mesh->GetIndexBuffer(), RHIIndexType::UInt32, 0);

            auto materialID = material->GetMaterialID();

            cmd->PushConstants(
                GetResourceBindingRegistry()->GetShaderResourceSignature(shader->GetUUID(), shader->GetSourceVersion()),
                RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,
                0,
                64,
                &renderObject->transform);

            cmd->PushConstants(
                GetResourceBindingRegistry()->GetShaderResourceSignature(shader->GetUUID(), shader->GetSourceVersion()),
                RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,
                64,
                4,
                &materialID);

            uint32_t entityID = renderObject->enttEntity;

            cmd->PushConstants(
                GetResourceBindingRegistry()->GetShaderResourceSignature(shader->GetUUID(), shader->GetSourceVersion()),
                RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,
                68,
                4,
                &entityID);

            cmd->SetViewport(
                viewportArea.offset.x, viewportArea.offset.y, viewportArea.extent.width, viewportArea.extent.height);
            cmd->SetScissor(
                scissorArea.offset.x, scissorArea.offset.y, scissorArea.extent.width, scissorArea.extent.height);
            cmd->SetBlendConstants(0.0f, 0.0f, 0.0f, 0.0f);

            cmd->DrawIndexed(mesh->GetIndices().size(), 1, 0, 0, 0);
        }

        cmd->EndRendering();
    }

    void Renderer::Release()
    {
        m_Device->WaitIdle();

        m_ResourceBindingRegistry->ClearAllResources();
        m_GPUAssetRegistry.reset();
        m_ResourceBindingRegistry.reset();
        m_GeometryDataRegistry.reset();
        m_ResourceHeapAllocator.reset();
    }
} // namespace Hazel
