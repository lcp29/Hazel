#include "Hazel/Renderer/Renderer.h"

#include "Hazel/Project/GlobalSettingRegistry.h"
#include "Hazel/Renderer/RenderTexture.h"
#include "Hazel/Renderer/Sampler.h"
#include "Hazel/Renderer/Texture.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    Renderer::Renderer(GraphicsContext* graphicsContext, Window* window)
        : m_GraphicsContext(graphicsContext)
          , m_Instance(graphicsContext->GetInstance())
          , m_Device(graphicsContext->GetDevice())
          , m_Window(window)
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

        CreateSwapchainResources();
        CreatePerFrameData();
        RecreateDefaultRenderTexture();
        CreateDefaultResources();
    }

    RenderTexture* Renderer::AddRenderTexture(std::unique_ptr<RenderTexture> renderTexture)
    {
        return m_OffscreenRenderTextures.Register(std::move(renderTexture));
    }

    void Renderer::RemoveRenderTexture(RenderTexture* renderTexture)
    {
        if (renderTexture)
            renderTexture->Release();
        m_OffscreenRenderTextures.Unregister(renderTexture);
    }

    Sampler* Renderer::AddSampler(std::unique_ptr<Sampler> sampler)
    {
        return m_Samplers.Register(std::move(sampler));
    }

    void Renderer::RemoveSampler(Sampler* sampler)
    {
        if (sampler)
            sampler->Release();
        m_Samplers.Unregister(sampler);
    }

    Texture* Renderer::AddTexture(std::unique_ptr<Texture> texture)
    {
        return m_Textures.Register(std::move(texture));
    }

    void Renderer::RemoveTexture(Texture* texture)
    {
        if (texture)
            texture->Release();
        m_Textures.Unregister(texture);
    }

    ComputeShader* Renderer::AddComputeShader(std::unique_ptr<ComputeShader> computeShader)
    {
        return m_ComputeShaders.Register(std::move(computeShader));
    }

    void Renderer::RemoveComputeShader(ComputeShader* computeShader)
    {
        if (computeShader)
            computeShader->Release();
        m_ComputeShaders.Unregister(computeShader);
    }

    void Renderer::CreateDefaultResources()
    {
        auto cmd = m_GraphicsContext->GetDefaultCommandBuffer();

        // error texture
        uint8_t data[4] = {255, 0, 255, 255};
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

        m_ErrorTexture = std::make_unique<Texture>(m_ErrorTextureUUID, textureDesc, this, image, imageView);

        RHISamplerDesc samplerDesc{};
        auto sampler = m_Device->CreateSampler(samplerDesc);
        m_DefaultSampler = std::make_unique<Sampler>(m_DefaultSamplerUUID, this, samplerDesc, sampler);
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
        auto* imageView = m_DefaultRenderTexture->GetImageView();
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

        m_DefaultRenderTexture = std::make_unique<RenderTexture>(m_DefaultRenderTextureUUID, this, renderTextureDesc);
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
} // namespace Hazel