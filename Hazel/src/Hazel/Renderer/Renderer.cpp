#include "Hazel/Renderer/Renderer.h"

#include "Hazel/Project/GlobalSettingRegistry.h"
#include "Hazel/Renderer/RenderTexture.h"

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
            m_Instance->GetHandle(), static_cast<GLFWwindow*>(m_Window->GetNativeWindow()), nullptr, &surface);
#endif
        m_WindowSurface = m_GraphicsContext->GetSurface();

        m_MaxFramesInFlight = GlobalSettings.Get(MaxFramesInFlightString, m_MaxFramesInFlight);
        m_SwapchainImageFormat = GlobalSettings.Get(SwapchainFormatString, m_SwapchainImageFormat);

        CreateSwapchainResources();
        CreatePerFrameData();
        RecreateDefaultRenderTexture();
    }

    RenderTexture* Renderer::AddRenderTexture(const RenderTextureDesc& desc)
    {
        auto renderTexture = std::make_unique<RenderTexture>(this, desc);
        return m_OffscreenRenderTextures.Register(std::move(renderTexture));
    }

    void Renderer::RemoveRenderTexture(RenderTexture* renderTexture)
    {
        m_OffscreenRenderTextures.Unregister(renderTexture);
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
        (void)camera;
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
        RenderTextureDesc renderTextureDesc{};
        renderTextureDesc.width = m_ViewportWidth;
        renderTextureDesc.height = m_ViewportHeight;
        renderTextureDesc.format = m_SwapchainImageFormat;
        renderTextureDesc.perFrame = true;
        renderTextureDesc.useMipmap = false;

        if (m_DefaultRenderTexture&& m_DefaultRenderTexture
        
        ->
        IsValid()
        )
        {
            m_DefaultRenderTexture->ReleaseImmediate();
            m_DefaultRenderTexture.reset();
        }

        m_DefaultRenderTexture = std::make_unique<RenderTexture>(this, renderTextureDesc);
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