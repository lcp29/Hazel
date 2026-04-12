//
// Created by helmholtz on 2026/3/14.
//

#include "VulkanCommandPool.h"

#include "../RHICommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_FUNC_IMPL(RHICommandPool, RHICommandPoolImpl)(RHIDevice* deviceOwner,
                                                         vk::Device device,
                                                         const RHICommandPoolDesc& desc,
                                                         uint32_t queueFamilyIndex)
        : m_DeviceOwner(deviceOwner)
        , m_Desc(desc)
        , m_Device(device)
        , m_QueueFamilyIndex(queueFamilyIndex)
    {
        vk::CommandPoolCreateInfo createInfo;
        createInfo.queueFamilyIndex = queueFamilyIndex;
        if (desc.transient) { createInfo.flags |= vk::CommandPoolCreateFlagBits::eTransient; }
        if (desc.allowCommandBufferReset) { createInfo.flags |= vk::CommandPoolCreateFlagBits::eResetCommandBuffer; }

        m_CommandPool = m_Device.createCommandPool(createInfo);
        m_IsValid = static_cast<bool>(m_CommandPool);
    }

    RHI_VK_FUNC_IMPL(RHICommandPool, ~RHICommandPoolImpl)() { Release(); }

    RHICommandBuffer* RHI_VK_FUNC_IMPL(RHICommandPool, CreateCommandBuffer)(const RHICommandBufferDesc& desc,
                                                                            bool isDetached)
    {
        if (!m_IsValid) { return nullptr; }

        std::unique_ptr<RHICommandBuffer> commandBuffer(new RHICommandBuffer(this, m_Device, m_CommandPool, desc));
        if (!commandBuffer || !commandBuffer->IsValid()) { return nullptr; }

        commandBuffer->m_IsDetached = isDetached;
        RHICommandBuffer* commandBufferPtr = commandBuffer.get();
        if (!isDetached)
            RegisterCommandBuffer(std::move(commandBuffer));
        else
            commandBuffer.release();
        return commandBufferPtr;
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, Release)()
    {
        if (!m_IsValid) { return; }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner && !m_IsDetached) { deviceOwner->UnregisterCommandPool(this); }
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, ReleaseImmediate)()
    {
        if (!m_IsValid) { return; }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner && !m_IsDetached) { deviceOwner->UnregisterCommandPool(this); }
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, ReleaseWithoutUnregister)()
    {
        if (!m_IsValid) { return; }

        for (const auto& commandBuffer : m_CommandBuffers)
        {
            if (commandBuffer) { commandBuffer->ReleaseWithoutUnregister(); }
        }
        m_CommandBuffers.Clear();

        const auto device = m_Device;
        const auto commandPool = m_CommandPool;
        auto pendingOperations = m_DeletionQueue.ExtractAll();
        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion(
                [device, commandPool, pendingOperations = std::move(pendingOperations)]() mutable {
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
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, ReleaseImmediateWithoutUnregister)()
    {
        if (!m_IsValid) { return; }

        for (const auto& commandBuffer : m_CommandBuffers)
        {
            if (commandBuffer) { commandBuffer->ReleaseImmediateWithoutUnregister(); }
        }
        m_CommandBuffers.Clear();

        auto pendingOperations = m_DeletionQueue.ExtractAll();
        DeletionQueue::Execute(std::move(pendingOperations));

        if (m_Device && m_CommandPool) { m_Device.destroyCommandPool(m_CommandPool); }

        m_CommandPool = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_QueueFamilyIndex = 0;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, EnqueueDeletion)(DeletionQueue::Operation operation)
    {
        m_DeletionQueue.Enqueue(std::move(operation));
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, RegisterCommandBuffer)(std::unique_ptr<RHICommandBuffer> commandBuffer)
    {
        m_CommandBuffers.Register(std::move(commandBuffer));
    }

    void RHI_VK_FUNC_IMPL(RHICommandPool, UnregisterCommandBuffer)(RHICommandBuffer* commandBuffer)
    {
        m_CommandBuffers.Unregister(commandBuffer);
    }
} // namespace Hazel