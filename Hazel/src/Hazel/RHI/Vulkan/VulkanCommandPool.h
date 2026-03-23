//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHICommandPool)
    {
    public:
        bool IsValid() const
        {
            return m_IsValid;
        }

        RHICommandBuffer* CreateCommandBuffer(const RHICommandBufferDesc& desc, bool isDetached = false);
        void Release();
        void ReleaseImmediate();
        ~RHICommandPoolImpl();

        const RHICommandPoolDesc& GetDesc() const
        {
            return m_Desc;
        }

        vk::CommandPool GetHandle() const
        {
            return m_CommandPool;
        }

        uint32_t GetQueueFamilyIndex() const
        {
            return m_QueueFamilyIndex;
        }

        bool IsDetached() const
        {
            return m_IsDetached;
        }

        void EnqueueDeletion(DeletionQueue::Operation operation);

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHICommandBufferImpl<RHIBackend::Vulkan>;

        RHICommandPoolImpl(RHIDevice* deviceOwner,
                           vk::Device device,
                           const RHICommandPoolDesc& desc,
                           uint32_t queueFamilyIndex);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();
        void RegisterCommandBuffer(std::unique_ptr<RHICommandBuffer> commandBuffer);
        void UnregisterCommandBuffer(RHICommandBuffer* commandBuffer);

        bool m_IsValid = false;
        RHIDevice* m_DeviceOwner = nullptr;
        RHICommandPoolDesc m_Desc;
        vk::Device m_Device;
        vk::CommandPool m_CommandPool = VK_NULL_HANDLE;
        uint32_t m_QueueFamilyIndex = 0;
        bool m_IsDetached = false;
        DeletionQueue m_DeletionQueue;
        RHIOwnerSet<RHICommandBuffer> m_CommandBuffers;
    };
} // namespace Hazel