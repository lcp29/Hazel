//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"
#include "VulkanImageView.h"

#include <filesystem>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Hazel
{
    class VulkanMemoryAllocator;

    RHI_VK_CLASS_IMPL(RHIImage)
    {
    public:
        bool IsValid() const { return m_IsValid; }
        bool IsSwapchainImage() const { return m_IsSwapchainImage; }
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
        RHIImageResourceState GetCurrentState() const { return m_CurrentState; }
        ~RHIImageImpl();

        class Factory
        {
        public:
            static RHIImage *CreateFromRawData(RHIDevice *device,
                                               RHICommandBuffer *cmd,
                                               const RHIImageDesc &desc,
                                               const void *data,
                                               size_t dataSize,
                                               RHIQueue *queue = nullptr);
            static RHIImage *CreateFromFile(RHIDevice *device,
                                            RHICommandBuffer *cmd,
                                            const RHIImageDesc &desc,
                                            const std::filesystem::path &path,
                                            RHIQueue *queue = nullptr);
        };

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIImageViewImpl<RHIBackend::Vulkan>;
        friend class RHISwapchainImpl<RHIBackend::Vulkan>;
        friend class Factory;

        RHIImageImpl(RHIDevice *deviceOwner, VulkanMemoryAllocator *allocator, const RHIImageDesc &desc);
        RHIImageImpl(RHIDevice *deviceOwner,
                     const RHIImageDesc &desc,
                     vk::Image image,
                     bool isSwapchainImage);

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
        RHIImageResourceState m_CurrentState = RHIImageResourceState::Undefined;
        bool m_IsSwapchainImage = false;
    };
} // Hazel
