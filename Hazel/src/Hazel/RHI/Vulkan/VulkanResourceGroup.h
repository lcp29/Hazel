//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"
#include "VulkanImageView.h"
#include "VulkanResourceLayout.h"
#include "VulkanSampler.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHIResourceGroup)
    {
    public:
        bool IsValid() const
        {
            return m_IsValid;
        }

        bool WriteBuffer(
            uint32_t slot,
            RHIBuffer* buffer,
            uint64_t offset = 0,
            uint64_t range = 0,
            uint32_t arrayElement = 0);
        bool WriteBufferView(uint32_t slot, RHIBufferView* bufferView, uint32_t arrayElement = 0);
        bool WriteImageView(uint32_t slot,
                            RHIImageView* imageView,
                            RHIImageResourceState state = RHIImageResourceState::ShaderRead,
                            uint32_t arrayElement = 0);
        bool WriteSampler(uint32_t slot, RHISampler* sampler, uint32_t arrayElement = 0);
        bool WriteSamplerWithImage(uint32_t slot,
                                   RHISampler* sampler,
                                   RHIImageView* imageView,
                                   RHIImageResourceState state = RHIImageResourceState::ShaderRead,
                                   uint32_t arrayElement = 0);
        void Release();
        void ReleaseImmediate();
        ~RHIResourceGroupImpl();

        RHIResourceLayout* GetLayout() const;

        bool IsDetached() const
        {
            return m_IsDetached;
        }

        vk::DescriptorSet GetHandle() const
        {
            return m_DescriptorSet;
        }

    private:
        friend class RHIResourceHeapImpl<RHIBackend::Vulkan>;

        RHIResourceGroupImpl(RHIResourceHeap* heapOwner, vk::Device device, RHIResourceLayout* layoutOwner);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();
        const RHIResourceBindingSlotDesc* FindBinding(uint32_t slot) const;

        bool m_IsValid = false;
        RHIResourceHeap* m_HeapOwner = nullptr;
        RHIResourceLayout* m_LayoutOwner = nullptr;
        vk::Device m_Device;
        vk::DescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
        bool m_IsDetached = false;
    };
} // namespace Hazel