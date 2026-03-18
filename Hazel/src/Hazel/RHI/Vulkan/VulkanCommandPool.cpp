//
// Created by helmholtz on 2026/3/14.
//

#include <vulkan/vulkan.hpp>

#include "../RHICommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanCommandPool.h"

namespace Hazel
{
    RHI_VK_FUNC_IMPL(RHICommandPool, RHICommandPoolImpl)(RHIDevice *deviceOwner,
                                                         vk::Device device,
                                                         const RHICommandPoolDesc &desc,
                                                         uint32_t queueFamilyIndex)
        : m_DeviceOwner(deviceOwner), m_Desc(desc), m_Device(device), m_QueueFamilyIndex(queueFamilyIndex)
    {
        vk::CommandPoolCreateInfo createInfo;
        createInfo.queueFamilyIndex = queueFamilyIndex;
        if (desc.transient)
        {
            createInfo.flags |= vk::CommandPoolCreateFlagBits::eTransient;
        }
        if (desc.allowCommandBufferReset)
        {
            createInfo.flags |= vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        }

        m_CommandPool = m_Device.createCommandPool(createInfo);
        m_IsValid = static_cast<bool>(m_CommandPool);
    }

    RHI_VK_FUNC_IMPL(RHICommandPool, ~RHICommandPoolImpl)()
    {
        Release();
    }

    RHICommandBuffer *RHI_VK_FUNC_IMPL(RHICommandPool, CreateCommandBuffer)(const RHICommandBufferDesc &desc)
    {
        if (!m_IsValid)
        {
            return nullptr;
        }

        std::unique_ptr<RHICommandBuffer> commandBuffer(new RHICommandBuffer(this, m_Device, m_CommandPool, desc));
        if (!commandBuffer || !commandBuffer->IsValid())
        {
            return nullptr;
        }

        RHICommandBuffer *commandBufferPtr = commandBuffer.get();
        RegisterCommandBuffer(std::move(commandBuffer));
        return commandBufferPtr;
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterCommandPool(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterCommandPool(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, ReleaseWithoutUnregister)()
    {
        if (!m_IsValid)
        {
            return;
        }

        for (const auto &commandBuffer: m_CommandBuffers)
        {
            if (commandBuffer)
            {
                commandBuffer->ReleaseWithoutUnregister();
            }
        }
        m_CommandBuffers.clear();

        const auto device = m_Device;
        const auto commandPool = m_CommandPool;
        auto pendingOperations = m_DeletionQueue.ExtractAll();
        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device, commandPool, pendingOperations = std::move(pendingOperations)]() mutable
            {
                if (device && commandPool)
                {
                    DeletionQueue::Execute(std::move(pendingOperations));
                    device.destroyCommandPool(commandPool);
                }
            });
        }
        else if (device && commandPool)
        {
            DeletionQueue::Execute(std::move(pendingOperations));
            device.destroyCommandPool(commandPool);
        }

        m_CommandPool = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_QueueFamilyIndex = 0;
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, ReleaseImmediateWithoutUnregister)()
    {
        if (!m_IsValid)
        {
            return;
        }

        for (const auto &commandBuffer: m_CommandBuffers)
        {
            if (commandBuffer)
            {
                commandBuffer->ReleaseImmediateWithoutUnregister();
            }
        }
        m_CommandBuffers.clear();

        auto pendingOperations = m_DeletionQueue.ExtractAll();
        DeletionQueue::Execute(std::move(pendingOperations));

        if (m_Device && m_CommandPool)
        {
            m_Device.destroyCommandPool(m_CommandPool);
        }

        m_CommandPool = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_QueueFamilyIndex = 0;
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, EnqueueDeletion)(DeletionQueue::Operation operation)
    {
        m_DeletionQueue.Enqueue(std::move(operation));
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, RegisterCommandBuffer)(std::unique_ptr<RHICommandBuffer> commandBuffer)
    {
        RegisterOwnedObject(m_CommandBuffers, std::move(commandBuffer));
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, UnregisterCommandBuffer)(RHICommandBuffer *commandBuffer)
    {
        UnregisterOwnedObject(m_CommandBuffers, commandBuffer);
    }
} // namespace Hazel
