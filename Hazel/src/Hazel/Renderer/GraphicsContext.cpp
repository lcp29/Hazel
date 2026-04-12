#include "Hazel/Renderer/GraphicsContext.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    Scope<GraphicsContext> GraphicsContext::Create(std::string appName, Window* window)
    {
        return CreateScope<GraphicsContext>(appName, window);
    }

    GraphicsContext::~GraphicsContext() { ReleaseDefaultCommandBuffers(); }

    void GraphicsContext::Init(Window* window)
    {
        if (m_Initialized) { return; }

        // create Vulkan instance
        RHIInstanceDesc instanceDesc;
#ifdef RHI_USE_VULKAN
        instanceDesc.backend = RHIBackend::Vulkan;
#endif
        instanceDesc.appName = m_AppName;
        instanceDesc.appVersion = {1, 0, 0};
        instanceDesc.engineName = "Hazel Engine";
        instanceDesc.engineVersion = {1, 0, 0};
        instanceDesc.useCustomDebugMessenger = false;
#if defined(HZ_DEBUG)
        instanceDesc.useValidation = true;
        instanceDesc.debugMessageSeverity = DebugMessageSeverityFlagBits::Error | DebugMessageSeverityFlagBits::Warning
                                            | DebugMessageSeverityFlagBits::Info;
        instanceDesc.debugMessageType = DebugMessageTypeFlagBits::General | DebugMessageTypeFlagBits::Validation
                                        | DebugMessageTypeFlagBits::Performance;
#else
        instanceDesc.useValidation = false;
        instanceDesc.debugMessageSeverity = {};
        instanceDesc.debugMessageType = {};
#endif

        m_Instance = std::make_unique<RHIInstance>(instanceDesc);

#if defined(RHI_USE_VULKAN)
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(
            m_Instance->GetHandle(), static_cast<GLFWwindow*>(m_Window->GetNativeWindow()), nullptr, &surface);
        RHISurfaceDesc surfaceDesc{surface};
#endif

        m_Surface = m_Instance->CreateSurface(surfaceDesc);

        // enumerate physical devices and create device
        RHIDeviceCapabilities deviceCaps;
        deviceCaps.queueTypes = RHIQueueTypeFlagBits::Graphics | RHIQueueTypeFlagBits::Compute
                                | RHIQueueTypeFlagBits::Transfer | RHIQueueTypeFlagBits::Present;
        deviceCaps.supportSubgroup = true;
        auto adapters = m_Instance->GetAdapters();
        for (auto& adapter : adapters)
        {
            HZ_CORE_INFO("Adapter: {0}", adapter.GetName());
            if (adapter.CanCreateDevice(deviceCaps))
            {
                m_Adapter = adapter;
                break;
            }
        }
        m_Device = m_Instance->CreateDevice(&m_Adapter, deviceCaps, m_Surface);

        m_Initialized = true;
    }

    RHICommandBuffer* GraphicsContext::AcquireDefaultCommandBuffer()
    {
        std::lock_guard lock(m_DefaultCommandBufferPoolMutex);
        for (auto& pooledCommandBuffer : m_DefaultCommandBuffers)
        {
            if (!pooledCommandBuffer.inUse)
            {
                pooledCommandBuffer.inUse = true;
                pooledCommandBuffer.commandBuffer->Reset();
                return pooledCommandBuffer.commandBuffer;
            }
        }

        RHICommandPoolDesc cmdPoolDesc{};
        cmdPoolDesc.allowCommandBufferReset = true;
        cmdPoolDesc.transient = false;
        auto* commandPool = m_Device->CreateCommandPoolUniformQueue(cmdPoolDesc);

        RHICommandBufferDesc cmdDesc{};
        cmdDesc.level = RHICommandBufferLevel::Primary;
        auto* commandBuffer = commandPool->CreateCommandBuffer(cmdDesc);

        auto& pooledCommandBuffer = m_DefaultCommandBuffers.emplace_back();
        pooledCommandBuffer.pool = commandPool;
        pooledCommandBuffer.commandBuffer = commandBuffer;
        pooledCommandBuffer.inUse = true;
        pooledCommandBuffer.commandBuffer->Reset();
        return pooledCommandBuffer.commandBuffer;
    }

    void GraphicsContext::ReleaseDefaultCommandBuffer(RHICommandBuffer* commandBuffer)
    {
        if (!commandBuffer) { return; }

        std::lock_guard lock(m_DefaultCommandBufferPoolMutex);
        for (auto& pooledCommandBuffer : m_DefaultCommandBuffers)
        {
            if (pooledCommandBuffer.commandBuffer == commandBuffer)
            {
                pooledCommandBuffer.inUse = false;
                return;
            }
        }
    }

    void GraphicsContext::ReleaseDefaultCommandBuffers()
    {
        std::lock_guard lock(m_DefaultCommandBufferPoolMutex);
        for (auto& pooledCommandBuffer : m_DefaultCommandBuffers)
        {
            if (pooledCommandBuffer.commandBuffer) { pooledCommandBuffer.commandBuffer->ReleaseImmediate(); }
            if (pooledCommandBuffer.pool) { pooledCommandBuffer.pool->ReleaseImmediate(); }
        }
        m_DefaultCommandBuffers.clear();
    }

    GraphicsContext::GraphicsContext(const std::string& appName, Window* window)
        : m_AppName(appName)
        , m_Window(window)
    {
        Init(window);
    }
} // namespace Hazel