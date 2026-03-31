#pragma once

#if defined(RHI_USE_VULKAN) && !defined(VK_VERSION_1_0)
#define VK_VERSION_1_0
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef VK_VERSION_1_0
#else
#include <GLFW/glfw3.h>
#endif

#include "Hazel/Core/Window.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class GraphicsContext
    {
    public:
        GraphicsContext(const std::string& appName, Window* window);
        ~GraphicsContext() = default;

        void Init(Window* window);

        RHIInstance* GetInstance() const
        {
            return m_Instance.get();
        }

        RHIAdapter GetAdapter() const
        {
            return m_Adapter;
        }

        RHIDevice* GetDevice() const
        {
            return m_Device;
        }

        RHISurface* GetSurface() const
        {
            return m_Surface;
        }

        static Scope<GraphicsContext> Create(std::string appName, Window* window);

        RHICommandBuffer* GetDefaultCommandBuffer() const
        {
            m_DefaultCommandBuffer->Reset();
            return m_DefaultCommandBuffer;
        }

    private:
        bool m_Initialized = false;
        std::string m_AppName;
        Window* m_Window = nullptr;
        RHISurface* m_Surface = nullptr;

        std::unique_ptr<RHIInstance> m_Instance;
        RHIAdapter m_Adapter;
        RHIDevice* m_Device = nullptr;

        RHICommandPool* m_DefaultCommandPool;
        // Do not use with multiple threads!!!
        RHICommandBuffer* m_DefaultCommandBuffer;
    };
} // namespace Hazel