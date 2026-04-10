//
// Created by helmholtz on 2026/3/15.
//

#include "VulkanResourceHeap.h"

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanResourceGroup.h"
#include "VulkanResourceLayout.h"

namespace Hazel
{
    RHI_VK_FUNC_IMPL(RHIResourceHeap, RHIResourceHeapImpl)(RHIDevice* deviceOwner,
                                                           vk::Device device,
                                                           const RHIResourceHeapDesc& desc)
    {
        m_DeviceOwner = deviceOwner;
        m_Device = device;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_Device || desc.maxGroups == 0)
        {
            return;
        }

        std::vector<vk::DescriptorPoolSize> poolSizes;
        if (desc.samplerCount > 0)
        {
            poolSizes.emplace_back(vk::DescriptorType::eSampler, desc.samplerCount);
        }
        if (desc.samplerWithImageCount > 0)
        {
            poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, desc.samplerWithImageCount);
        }
        if (desc.sampledImageCount > 0)
        {
            poolSizes.emplace_back(vk::DescriptorType::eSampledImage, desc.sampledImageCount);
        }
        if (desc.storageImageCount > 0)
        {
            poolSizes.emplace_back(vk::DescriptorType::eStorageImage, desc.storageImageCount);
        }
        if (desc.uniformBufferCount > 0)
        {
            poolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, desc.uniformBufferCount);
        }
        if (desc.storageBufferCount > 0)
        {
            poolSizes.emplace_back(vk::DescriptorType::eStorageBuffer, desc.storageBufferCount);
        }
        if (desc.uniformTexelBufferCount > 0)
        {
            poolSizes.emplace_back(vk::DescriptorType::eUniformTexelBuffer, desc.uniformTexelBufferCount);
        }
        if (desc.storageTexelBufferCount > 0)
        {
            poolSizes.emplace_back(vk::DescriptorType::eStorageTexelBuffer, desc.storageTexelBufferCount);
        }

        if (poolSizes.empty())
        {
            return;
        }

        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

        if (desc.updateAfterBind)
        {
            descriptorPoolCreateInfo.flags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
        }

        descriptorPoolCreateInfo.maxSets = desc.maxGroups;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

        m_DescriptorPool = m_Device.createDescriptorPool(descriptorPoolCreateInfo);
        m_IsValid = static_cast<bool>(m_DescriptorPool);
    }

    RHI_VK_FUNC_IMPL(RHIResourceHeap, ~RHIResourceHeapImpl)()
    {
        Release();
    }

    RHIResourceGroup* RHI_VK_FUNC_IMPL(RHIResourceHeap, CreateGroup)(RHIResourceLayout* layout, bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_IsValid || !layout || !layout->IsValid());

        std::unique_ptr<RHIResourceGroup> group(new RHIResourceGroup(this, m_Device, layout));
        if (!group || !group->IsValid())
        {
            return nullptr;
        }

        group->m_IsDetached = isDetached;
        auto* groupPtr = group.get();
        if (!isDetached)
            RegisterGroup(std::move(group));
        else
            group.release();
        return groupPtr;
    }

    void RHI_VK_FUNC_IMPL(RHIResourceHeap, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner && !m_IsDetached)
        {
            deviceOwner->UnregisterResourceHeap(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIResourceHeap, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner && !m_IsDetached)
        {
            deviceOwner->UnregisterResourceHeap(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIResourceHeap, ReleaseWithoutUnregister)()
    {
        for (const auto& group : m_Groups)
        {
            if (group)
            {
                group->ReleaseWithoutUnregister();
            }
        }
        m_Groups.Clear();

        const auto device = m_Device;
        const auto descriptorPool = m_DescriptorPool;
        auto pendingOperations = m_DeletionQueue.ExtractAll();
        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion(
                [device, descriptorPool, pendingOperations = std::move(pendingOperations)]() mutable {
                    if (device && descriptorPool)
                    {
                        DeletionQueue::Execute(std::move(pendingOperations));
                        device.destroyDescriptorPool(descriptorPool);
                    }
                });
        }
        else if (device && descriptorPool)
        {
            DeletionQueue::Execute(std::move(pendingOperations));
            device.destroyDescriptorPool(descriptorPool);
        }

        m_DescriptorPool = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHIResourceHeap, ReleaseImmediateWithoutUnregister)()
    {
        for (const auto& group : m_Groups)
        {
            if (group)
            {
                group->ReleaseImmediateWithoutUnregister();
            }
        }
        m_Groups.Clear();

        auto pendingOperations = m_DeletionQueue.ExtractAll();
        DeletionQueue::Execute(std::move(pendingOperations));

        if (m_Device && m_DescriptorPool)
        {
            m_Device.destroyDescriptorPool(m_DescriptorPool);
        }

        m_DescriptorPool = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHIResourceHeap, RegisterGroup)(std::unique_ptr<RHIResourceGroup> group)
    {
        m_Groups.Register(std::move(group));
    }

    void RHI_VK_FUNC_IMPL(RHIResourceHeap, UnregisterGroup)(RHIResourceGroup* group)
    {
        m_Groups.Unregister(group);
    }
} // namespace Hazel