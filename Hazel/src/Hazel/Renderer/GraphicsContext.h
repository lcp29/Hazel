#pragma once

// ======== Aster Modify Begin ========
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

#include <mutex>
#include <vector>

// ======== Aster Modify End ========

namespace Hazel
{
    class GraphicsContext
    {
      public:
        // ======== Aster Modify Begin ========
        GraphicsContext(const std::string& appName, Window* window);
        ~GraphicsContext();

        void Init(Window* window);

        Aster::RHIInstance* GetInstance() const { return m_Instance.get(); }

        Aster::RHIAdapter GetAdapter() const { return m_Adapter; }

        Aster::RHIDevice* GetDevice() const { return m_Device; }

        Aster::RHISurface* GetSurface() const { return m_Surface; }

        static Scope<GraphicsContext> Create(std::string appName, Window* window);

        Aster::RHICommandBuffer* AcquireDefaultCommandBuffer();
        void ReleaseDefaultCommandBuffer(Aster::RHICommandBuffer* commandBuffer);
        void ReleaseDefaultCommandBuffers();

      private:
        struct PooledCommandBuffer
        {
            Aster::RHICommandPool* pool = nullptr;
            Aster::RHICommandBuffer* commandBuffer = nullptr;
            bool inUse = false;
        };

        bool m_Initialized = false;
        std::string m_AppName;
        Window* m_Window = nullptr;
        Aster::RHISurface* m_Surface = nullptr;

        std::unique_ptr<Aster::RHIInstance> m_Instance;
        Aster::RHIAdapter m_Adapter;
        Aster::RHIDevice* m_Device = nullptr;

        mutable std::mutex m_DefaultCommandBufferPoolMutex;
        std::vector<PooledCommandBuffer> m_DefaultCommandBuffers;
    };
} // namespace Hazel

// ======== Aster Modify End ========
