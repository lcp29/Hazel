//
// Created by helmholtz on 2026/3/14.
//

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "../RHIImage.h"
#include "VulkanImage.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanMemoryAllocator.h"

namespace Hazel
{
    namespace
    {
        RHIImagePlanes GetDefaultTransitionPlanes(RHIFormat format)
        {
            switch (format)
            {
                case RHIFormat::D32SFloat:
                    return RHIImagePlaneFlagBits::Depth;
                case RHIFormat::D32SFloatS8Uint:
                    return RHIImagePlaneFlagBits::Depth | RHIImagePlaneFlagBits::Stencil;
                case RHIFormat::S8Uint:
                    return RHIImagePlaneFlagBits::Stencil;
                default:
                    return RHIImagePlaneFlagBits::Color;
            }
        }

        RHIPipelineStages GetPipelineStagesForState(RHIImageResourceState state)
        {
            switch (state)
            {
                case RHIImageResourceState::Undefined:
                    return RHIPipelineStageFlagBits::Top;
                case RHIImageResourceState::Common:
                    return RHIPipelineStageFlagBits::AllCommands;
                case RHIImageResourceState::TransferSource:
                case RHIImageResourceState::TransferDestination:
                    return RHIPipelineStageFlagBits::Transfer;
                case RHIImageResourceState::ShaderRead:
                case RHIImageResourceState::ShaderWrite:
                    return RHIPipelineStageFlagBits::VertexShader
                           | RHIPipelineStageFlagBits::FragmentShader
                           | RHIPipelineStageFlagBits::ComputeShader;
                case RHIImageResourceState::ColorAttachment:
                    return RHIPipelineStageFlagBits::ColorAttachmentOutput;
                case RHIImageResourceState::DepthStencilAttachment:
                    return RHIPipelineStageFlagBits::EarlyDepthStencil
                           | RHIPipelineStageFlagBits::LateDepthStencil;
                case RHIImageResourceState::Present:
                    return RHIPipelineStageFlagBits::Bottom;
            }

            return RHIPipelineStageFlagBits::AllCommands;
        }

        RHIPipelineAccessFlags GetAccessFlagsForState(RHIImageResourceState state)
        {
            switch (state)
            {
                case RHIImageResourceState::Undefined:
                case RHIImageResourceState::Present:
                    return {};
                case RHIImageResourceState::Common:
                    return RHIPipelineAccessFlagBits::MemoryRead | RHIPipelineAccessFlagBits::MemoryWrite;
                case RHIImageResourceState::TransferSource:
                    return RHIPipelineAccessFlagBits::TransferRead;
                case RHIImageResourceState::TransferDestination:
                    return RHIPipelineAccessFlagBits::TransferWrite;
                case RHIImageResourceState::ShaderRead:
                    return RHIPipelineAccessFlagBits::ShaderRead;
                case RHIImageResourceState::ShaderWrite:
                    return RHIPipelineAccessFlagBits::ShaderWrite;
                case RHIImageResourceState::ColorAttachment:
                    return RHIPipelineAccessFlagBits::ColorAttachmentRead
                           | RHIPipelineAccessFlagBits::ColorAttachmentWrite;
                case RHIImageResourceState::DepthStencilAttachment:
                    return RHIPipelineAccessFlagBits::DepthStencilAttachmentRead
                           | RHIPipelineAccessFlagBits::DepthStencilAttachmentWrite;
            }

            return {};
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHIImage, RHIImageImpl)(RHIDevice *deviceOwner,
                                             VulkanMemoryAllocator *allocator,
                                             const RHIImageDesc &desc)
    {
        m_DeviceOwner = deviceOwner;
        m_AllocatorOwner = allocator;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_AllocatorOwner || desc.format == RHIFormat::Undefined)
        {
            return;
        }

        vk::ImageCreateInfo imageCreateInfo;
        imageCreateInfo.imageType = desc.depth > 1 ? vk::ImageType::e3D : vk::ImageType::e2D;
        imageCreateInfo.format = VulkanConvertFormat(desc.format);
        imageCreateInfo.extent = vk::Extent3D(desc.width, desc.height, desc.depth);
        imageCreateInfo.mipLevels = desc.mipLevels;
        imageCreateInfo.arrayLayers = desc.arrayLayers;
        imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        imageCreateInfo.usage = VulkanConvertImageUsages(desc.usages);
        imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        imageCreateInfo.initialLayout = VulkanConvertResourceState(desc.initialState);

        if (imageCreateInfo.usage == vk::ImageUsageFlags())
        {
            imageCreateInfo.usage = vk::ImageUsageFlagBits::eSampled;
        }

        if (desc.arrayLayers == 6 && desc.width == desc.height)
        {
            imageCreateInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
        }

        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VkImage image = VK_NULL_HANDLE;
        VkImageCreateInfo vkImageCreateInfo = imageCreateInfo;
        if (!m_AllocatorOwner->CreateImage(
                vkImageCreateInfo,
                allocationCreateInfo,
                &image,
                &m_Allocation))
        {
            m_Allocation = VK_NULL_HANDLE;
            return;
        }

        m_Image = image;
        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIImage, ~RHIImageImpl)()
    {
        Release();
    }

    void RHI_VK_FUNC_IMPL(RHIImage, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterImage(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIImage, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterImage(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIImage, ReleaseWithoutUnregister)()
    {
        auto allocator = m_AllocatorOwner ? m_AllocatorOwner->GetHandle() : VK_NULL_HANDLE;
        const auto image = static_cast<VkImage>(m_Image);
        const auto allocation = m_Allocation;

        for (const auto &view: m_Views)
        {
            if (view)
            {
                view->ReleaseWithoutUnregister();
            }
        }
        m_Views.clear();

        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([allocator, image, allocation]()
            {
                VulkanMemoryAllocator::DestroyImage(allocator, image, allocation);
            });
        }
        else
        {
            VulkanMemoryAllocator::DestroyImage(allocator, image, allocation);
        }

        m_Image = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_AllocatorOwner = nullptr;
    }

    void RHI_VK_FUNC_IMPL(RHIImage, ReleaseImmediateWithoutUnregister)()
    {
        auto allocator = m_AllocatorOwner ? m_AllocatorOwner->GetHandle() : VK_NULL_HANDLE;
        const auto image = static_cast<VkImage>(m_Image);
        const auto allocation = m_Allocation;

        for (const auto &view: m_Views)
        {
            if (view)
            {
                view->ReleaseImmediateWithoutUnregister();
            }
        }
        m_Views.clear();

        VulkanMemoryAllocator::DestroyImage(allocator, image, allocation);

        m_Image = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_AllocatorOwner = nullptr;
    }

    RHIImageView *RHI_VK_FUNC_IMPL(RHIImage, CreateView)(const RHIImageViewDesc &desc)
    {
        if (!m_IsValid || !m_DeviceOwner)
        {
            return nullptr;
        }

        return m_DeviceOwner->CreateImageView(this, desc);
    }

    bool RHI_VK_FUNC_IMPL(RHIImage, Transition)(RHICommandBuffer *commandBuffer,
                                                RHIImageResourceState oldState,
                                                RHIImageResourceState newState)
    {
        RHIImageSubresourceRange fullRange;
        fullRange.levelCount = m_Desc.mipLevels;
        fullRange.layerCount = m_Desc.arrayLayers;
        fullRange.planes = GetDefaultTransitionPlanes(m_Desc.format);
        return Transition(commandBuffer, oldState, newState, fullRange);
    }

    bool RHI_VK_FUNC_IMPL(RHIImage, Transition)(RHICommandBuffer *commandBuffer,
                                                RHIImageResourceState oldState,
                                                RHIImageResourceState newState,
                                                const RHIImageSubresourceRange &subresourceRange,
                                                RHIQueue *srcQueue,
                                                RHIQueue *dstQueue)
    {
        if (!m_IsValid || !commandBuffer || !commandBuffer->IsValid())
        {
            return false;
        }
        if (subresourceRange.levelCount == 0 || subresourceRange.layerCount == 0)
        {
            return false;
        }

        RHIPipelineBarrierDesc barrierDesc;
        barrierDesc.imageBarriers.push_back({
            this,
            GetPipelineStagesForState(oldState),
            GetPipelineStagesForState(newState),
            GetAccessFlagsForState(oldState),
            GetAccessFlagsForState(newState),
            oldState,
            newState,
            srcQueue,
            dstQueue,
            subresourceRange
        });
        return commandBuffer->PipelineBarriers(barrierDesc);
    }

    void RHI_VK_FUNC_IMPL(RHIImage, RegisterView)(std::unique_ptr<RHIImageView> view)
    {
        RegisterOwnedObject(m_Views, std::move(view));
    }

    void RHI_VK_FUNC_IMPL(RHIImage, UnregisterView)(RHIImageView *view)
    {
        UnregisterOwnedObject(m_Views, view);
    }
} // Hazel
