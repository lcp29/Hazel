//
// Created by helmholtz on 2026/3/13.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"
#include "VulkanCommon.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHIInstance)
    {
      public:
        bool IsValid() const { return m_IsValid; }

        std::vector<RHIAdapter> GetAdapters();
        RHISurface* CreateSurface(const RHISurfaceDesc& desc);
        RHIDevice* CreateDevice(
            const RHIAdapter* adapter, const RHIDeviceCapabilities& caps, const RHISurface* surface = nullptr);
        void Release();

        explicit RHIInstanceImpl(const RHIInstanceDesc& desc);
        RHIInstanceImpl(const RHIInstance&) = delete;
        RHIInstanceImpl& operator=(const RHIInstance&) = delete;
        RHIInstanceImpl(RHIInstance&&) noexcept;
        ~RHIInstanceImpl();

        vk::Instance GetHandle() const { return m_Instance; }

        void EnqueueDeletion(DeletionQueue::Operation operation) { m_DeletionQueue.Enqueue(std::move(operation)); }

        void FlushDeletionQueue() { m_DeletionQueue.Flush(); }

        DeletionQueue::OperationSet ExtractDeletionQueue() { return m_DeletionQueue.ExtractAll(); }

      private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHISurfaceImpl<RHIBackend::Vulkan>;

        void RegisterDevice(std::unique_ptr<RHIDevice> device);
        void UnregisterDevice(RHIDevice * device);
        void RegisterSurface(std::unique_ptr<RHISurface> surface);
        void UnregisterSurface(RHISurface * surface);

        bool m_IsValid = false;
        vk::Instance m_Instance;
        RHIInstanceDesc m_InstanceDesc;
        VulkanDebugMessageContext m_DebugCallbackContext;
        vk::DebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        vk::detail::DispatchLoaderDynamic m_DynamicLoader;
        RHIOwnerSet<RHIDevice> m_Devices;
        RHIOwnerSet<RHISurface> m_Surfaces;
        DeletionQueue m_DeletionQueue;
    };
} // namespace Hazel