// Implements the Vulkan resource group backend.
// Created: 2026-03-15.

#include "VulkanResourceGroup.h"

#include "VulkanCommon.h"
#include "VulkanResourceHeap.h"

namespace Aster
{
    RHI_VK_FUNC_IMPL(RHIResourceGroup, RHIResourceGroupImpl)(RHIResourceHeap* heapOwner,
                                                             vk::Device device,
                                                             RHIResourceLayout* layoutOwner)
    {
        m_HeapOwner = heapOwner;
        m_LayoutOwner = layoutOwner;
        m_Device = device;

        if (!m_HeapOwner || !m_LayoutOwner || !m_Device) { return; }

        const auto descriptorSetLayout = m_LayoutOwner->GetDescriptorSetLayout();
        vk::DescriptorSetAllocateInfo allocateInfo;
        allocateInfo.descriptorPool = m_HeapOwner->GetHandle();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &descriptorSetLayout;

        const auto descriptorSets = m_Device.allocateDescriptorSets(allocateInfo);
        if (descriptorSets.empty()) { return; }

        m_DescriptorSet = descriptorSets.front();
        m_IsValid = static_cast<bool>(m_DescriptorSet);
    }

    RHI_VK_FUNC_IMPL(RHIResourceGroup, ~RHIResourceGroupImpl)() { Release(); }

    bool RHI_VK_FUNC_IMPL(RHIResourceGroup, WriteBuffer)(
        uint32_t slot, RHIBuffer* buffer, uint64_t offset, uint64_t range, uint32_t arrayElement)
    {
        const auto* bindingDesc = FindBinding(slot);
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !bindingDesc || !buffer || !buffer->IsValid());

        const auto resolvedRange = range == 0 ? buffer->GetSize() - offset : range;

        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = buffer->GetHandle();
        bufferInfo.offset = offset;
        bufferInfo.range = resolvedRange;

        vk::WriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.dstSet = m_DescriptorSet;
        writeDescriptorSet.dstBinding = slot;
        writeDescriptorSet.dstArrayElement = arrayElement;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VulkanConvertResourceBindingType(bindingDesc->type);
        writeDescriptorSet.pBufferInfo = &bufferInfo;

        m_Device.updateDescriptorSets(writeDescriptorSet, {});
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHIResourceGroup, WriteImageView)(uint32_t slot,
                                                            RHIImageView* imageView,
                                                            RHIImageResourceState state,
                                                            uint32_t arrayElement)
    {
        const auto* bindingDesc = FindBinding(slot);
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !bindingDesc || !imageView || !imageView->IsValid());

        vk::DescriptorImageInfo imageInfo;
        imageInfo.imageView = imageView->GetHandle();
        imageInfo.imageLayout = VulkanConvertImageResourceState(state);

        vk::WriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.dstSet = m_DescriptorSet;
        writeDescriptorSet.dstBinding = slot;
        writeDescriptorSet.dstArrayElement = arrayElement;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VulkanConvertResourceBindingType(bindingDesc->type);
        writeDescriptorSet.pImageInfo = &imageInfo;

        m_Device.updateDescriptorSets(writeDescriptorSet, {});
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHIResourceGroup, WriteSampler)(uint32_t slot, RHISampler* sampler, uint32_t arrayElement)
    {
        const auto* bindingDesc = FindBinding(slot);
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !bindingDesc || !sampler || !sampler->IsValid());

        vk::DescriptorImageInfo imageInfo;
        imageInfo.sampler = sampler->GetHandle();

        vk::WriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.dstSet = m_DescriptorSet;
        writeDescriptorSet.dstBinding = slot;
        writeDescriptorSet.dstArrayElement = arrayElement;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VulkanConvertResourceBindingType(bindingDesc->type);
        writeDescriptorSet.pImageInfo = &imageInfo;

        m_Device.updateDescriptorSets(writeDescriptorSet, {});
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHIResourceGroup, WriteSamplerWithImage)(
        uint32_t slot, RHISampler* sampler, RHIImageView* imageView, RHIImageResourceState state, uint32_t arrayElement)
    {
        const auto* bindingDesc = FindBinding(slot);
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !bindingDesc || !sampler || !imageView || !sampler->IsValid()
                             || !imageView->IsValid());

        vk::DescriptorImageInfo imageInfo;
        imageInfo.sampler = sampler->GetHandle();
        imageInfo.imageView = imageView->GetHandle();
        imageInfo.imageLayout = VulkanConvertImageResourceState(state);

        vk::WriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.dstSet = m_DescriptorSet;
        writeDescriptorSet.dstBinding = slot;
        writeDescriptorSet.dstArrayElement = arrayElement;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VulkanConvertResourceBindingType(bindingDesc->type);
        writeDescriptorSet.pImageInfo = &imageInfo;

        m_Device.updateDescriptorSets(writeDescriptorSet, {});
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHIResourceGroup,
                          WriteBufferView)(uint32_t slot, RHIBufferView* bufferView, uint32_t arrayElement)
    {
        const auto* bindingDesc = FindBinding(slot);
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !bindingDesc || !bufferView || !bufferView->IsValid());

        const auto bufferViewHandle = bufferView->GetHandle();
        vk::WriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.dstSet = m_DescriptorSet;
        writeDescriptorSet.dstBinding = slot;
        writeDescriptorSet.dstArrayElement = arrayElement;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VulkanConvertResourceBindingType(bindingDesc->type);
        writeDescriptorSet.pTexelBufferView = &bufferViewHandle;

        m_Device.updateDescriptorSets(writeDescriptorSet, {});
        return true;
    }

    void RHI_VK_FUNC_IMPL(RHIResourceGroup, Release)()
    {
        if (!m_IsValid) { return; }

        auto* heapOwner = m_HeapOwner;
        ReleaseWithoutUnregister();
        if (heapOwner && !m_IsDetached) { heapOwner->UnregisterGroup(this); }
    }

    void RHI_VK_FUNC_IMPL(RHIResourceGroup, ReleaseImmediate)()
    {
        if (!m_IsValid) { return; }

        auto* heapOwner = m_HeapOwner;
        ReleaseImmediateWithoutUnregister();
        if (heapOwner && !m_IsDetached) { heapOwner->UnregisterGroup(this); }
    }

    void RHI_VK_FUNC_IMPL(RHIResourceGroup, ReleaseWithoutUnregister)()
    {
        const auto device = m_Device;
        const auto descriptorPool = m_HeapOwner ? m_HeapOwner->GetHandle() : VK_NULL_HANDLE;
        const auto descriptorSet = m_DescriptorSet;
        if (m_HeapOwner)
        {
            m_HeapOwner->EnqueueDeletion([device, descriptorPool, descriptorSet]() {
                if (device && descriptorPool && descriptorSet)
                {
                    device.freeDescriptorSets(descriptorPool, descriptorSet);
                }
            });
        }
        else if (device && descriptorPool && descriptorSet)
        {
            device.freeDescriptorSets(descriptorPool, descriptorSet);
        }

        m_DescriptorSet = VK_NULL_HANDLE;
        m_IsValid = false;
        m_HeapOwner = nullptr;
        m_LayoutOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHIResourceGroup, ReleaseImmediateWithoutUnregister)()
    {
        if (m_Device && m_HeapOwner
            && m_HeapOwner

                   ->GetHandle()
            && m_DescriptorSet)
        {
            m_Device.freeDescriptorSets(m_HeapOwner->GetHandle(), m_DescriptorSet);
        }

        m_DescriptorSet = VK_NULL_HANDLE;
        m_IsValid = false;
        m_HeapOwner = nullptr;
        m_LayoutOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_IsDetached = false;
    }

    RHIResourceLayout* RHI_VK_FUNC_IMPL(RHIResourceGroup, GetLayout)() const { return m_LayoutOwner; }

    const RHIResourceBindingSlotDesc* RHI_VK_FUNC_IMPL(RHIResourceGroup, FindBinding)(uint32_t slot) const
    {
        if (!m_LayoutOwner) { return nullptr; }

        for (const auto& bindingDesc : m_LayoutOwner->GetDesc().bindings)
        {
            if (bindingDesc.slot == slot) { return &bindingDesc; }
        }

        return nullptr;
    }
} // namespace Aster
