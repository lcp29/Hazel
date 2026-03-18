//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"
#include "VulkanImageView.h"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Hazel
{
    class VulkanMemoryAllocator;

    RHI_VK_CLASS_IMPL(RHIImage)
    {
    public:
        bool IsValid() const { return m_IsValid; }
        void Release();
        void ReleaseImmediate();
        RHIImageView *CreateView(const RHIImageViewDesc &desc);
        bool Transition(RHICommandBuffer *commandBuffer,
                        RHIImageResourceState oldState,
                        RHIImageResourceState newState);
        bool Transition(RHICommandBuffer *commandBuffer,
                        RHIImageResourceState oldState,
                        RHIImageResourceState newState,
                        const RHIImageSubresourceRange &subresourceRange,
                        RHIQueue *srcQueue = nullptr,
                        RHIQueue *dstQueue = nullptr);
        const RHIImageDesc &GetDesc() const { return m_Desc; }

        vk::Image GetHandle() const { return m_Image; }
        VmaAllocation GetAllocation() const { return m_Allocation; }
        ~RHIImageImpl();

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIImageViewImpl<RHIBackend::Vulkan>;

        RHIImageImpl(RHIDevice *deviceOwner, VulkanMemoryAllocator *allocator, const RHIImageDesc &desc);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();
        void RegisterView(std::unique_ptr<RHIImageView> view);
        void UnregisterView(RHIImageView *view);

        bool m_IsValid = false;
        RHIImageDesc m_Desc;
        RHIDevice *m_DeviceOwner = nullptr;
        VulkanMemoryAllocator *m_AllocatorOwner = nullptr;
        vk::Image m_Image = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        RHIOwnerSet<RHIImageView> m_Views;
    };
} // Hazel
