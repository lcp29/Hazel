//
// Created by helmholtz on 2026/3/14.
//

#include "VulkanImage.h"

#include "../RHIImage.h"
#include "Hazel/Core/Log.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanMemoryAllocator.h"

#include <stb_image.h>
#include <utility>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Hazel
{
    namespace
    {
        uint32_t DeduceMipLevelCount(uint32_t width, uint32_t height)
        {
            uint32_t maxDimension = width > height ? width : height;
            uint32_t mipLevels = 1;
            while (maxDimension > 1)
            {
                maxDimension >>= 1;
                ++mipLevels;
            }

            return mipLevels;
        }

        uint32_t GetBytesPerPixel(RHIFormat format)
        {
            switch (format)
            {
                case RHIFormat::R8UNorm:
                    return 1;
                case RHIFormat::RG8UNorm:
                    return 2;
                case RHIFormat::RGB32SFloat:
                    return 12;
                case RHIFormat::RGBA8UNorm:
                case RHIFormat::RGBA8SRGB:
                case RHIFormat::BGRA8UNorm:
                case RHIFormat::BGRA8SRGB:
                    return 4;
                default:
                    return 0;
            }
        }

        RHIFormat DeduceNonHDRFormat(const int channels, const bool isSRGB)
        {
            switch (channels)
            {
                case 1:
                    return RHIFormat::R8UNorm;
                case 2:
                    return RHIFormat::RG8UNorm;
                case 3:
                case 4:
                    return isSRGB ? RHIFormat::RGBA8SRGB : RHIFormat::RGBA8UNorm;
                default:
                    return RHIFormat::Undefined;
            }
        }

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
                    return RHIPipelineStageFlagBits::VertexShader | RHIPipelineStageFlagBits::FragmentShader
                           | RHIPipelineStageFlagBits::ComputeShader;
                case RHIImageResourceState::ColorAttachment:
                    return RHIPipelineStageFlagBits::ColorAttachmentOutput;
                case RHIImageResourceState::DepthStencilAttachment:
                case RHIImageResourceState::DepthAttachment:
                case RHIImageResourceState::StencilAttachment:
                    return RHIPipelineStageFlagBits::EarlyDepthStencil | RHIPipelineStageFlagBits::LateDepthStencil;
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
                case RHIImageResourceState::DepthAttachment:
                case RHIImageResourceState::StencilAttachment:
                    return RHIPipelineAccessFlagBits::DepthStencilAttachmentRead
                           | RHIPipelineAccessFlagBits::DepthStencilAttachmentWrite;
            }

            return {};
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHIImage,
                     RHIImageImpl)(RHIDevice* deviceOwner, VulkanMemoryAllocator* allocator, const RHIImageDesc& desc)
    {
        m_DeviceOwner = deviceOwner;
        m_AllocatorOwner = allocator;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_AllocatorOwner || desc.format == RHIFormat::Undefined) { return; }

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
        imageCreateInfo.initialLayout = VulkanConvertImageResourceState(desc.initialState);

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
        if (!m_AllocatorOwner->CreateImage(vkImageCreateInfo, allocationCreateInfo, &image, &m_Allocation))
        {
            m_Allocation = VK_NULL_HANDLE;
            return;
        }

        m_Image = image;
        m_CurrentState = desc.initialState;
        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIImage,
                     RHIImageImpl)(RHIDevice* deviceOwner, const RHIImageDesc& desc, vk::Image image, bool isDetached)
    {
        m_DeviceOwner = deviceOwner;
        m_Desc = desc;
        m_Image = image;
        m_CurrentState = desc.initialState;
        m_IsSwapchainImage = true;
        m_IsDetached = isDetached;

        if (!m_DeviceOwner || !m_Image || desc.format == RHIFormat::Undefined) { return; }

        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIImage, ~RHIImageImpl)() { Release(); }

    RHIImage* RHI_VK_FUNC_IMPL(RHIImage, Factory)::CreateFromRawData(RHIDevice* device,
                                                                     RHICommandBuffer* cmd,
                                                                     const RHIImageDesc& desc,
                                                                     const void* data,
                                                                     size_t dataSize,
                                                                     bool detached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!device || !cmd || !cmd->IsValid() || !data || dataSize == 0);
        const uint32_t bytesPerPixel = GetBytesPerPixel(desc.format);
        HZ_RHI_DEBUG_RETURN_NULL_IF(bytesPerPixel == 0);

        RHIImageDesc imageDesc = desc;
        const RHIImageResourceState finalState = imageDesc.initialState == RHIImageResourceState::Undefined
                                                     ? RHIImageResourceState::TransferDestination
                                                     : imageDesc.initialState;
        imageDesc.initialState = RHIImageResourceState::Undefined;
        imageDesc.usages = imageDesc.usages | RHIImageUsageFlagBits::TransferDestination;

        auto* targetImage = device->CreateImage(imageDesc, detached);
        if (!targetImage || !targetImage->IsValid()) { return nullptr; }

        RHIBufferDesc stagingBufferDesc{};
        stagingBufferDesc.size = dataSize;
        stagingBufferDesc.usages = RHIBufferUsageFlagBits::TransferSource;
        stagingBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
        stagingBufferDesc.mapOnCreate = true;

        auto* stagingBuffer = device->CreateBuffer(stagingBufferDesc, true);
        if (!stagingBuffer || !stagingBuffer->IsValid())
        {
            if (stagingBuffer) { stagingBuffer->ReleaseImmediate(); }
            targetImage->ReleaseImmediate();
            return nullptr;
        }

        void* mappedData = stagingBuffer->Map();
        if (!mappedData)
        {
            stagingBuffer->ReleaseImmediate();
            targetImage->ReleaseImmediate();
            return nullptr;
        }

        std::memcpy(mappedData, data, dataSize);
        stagingBuffer->Unmap();

        if (!targetImage->Transition(cmd, RHIImageResourceState::Undefined, RHIImageResourceState::TransferDestination))
        {
            stagingBuffer->ReleaseImmediate();
            targetImage->ReleaseImmediate();
            return nullptr;
        }

        if (!cmd->CopyBufferToImage(stagingBuffer,
                                    0,
                                    {imageDesc.width, imageDesc.height},
                                    targetImage,
                                    {0, 0, 0},
                                    {imageDesc.width, imageDesc.height, 1},
                                    {0, 0, 1, GetDefaultTransitionPlanes(imageDesc.format)}))
        {
            stagingBuffer->ReleaseImmediate();
            targetImage->ReleaseImmediate();
            return nullptr;
        }

        if (finalState != RHIImageResourceState::TransferDestination)
        {
            if (!targetImage->Transition(cmd, RHIImageResourceState::TransferDestination, finalState))
            {
                stagingBuffer->ReleaseImmediate();
                targetImage->ReleaseImmediate();
                return nullptr;
            }
        }

        stagingBuffer->Release();
        return targetImage;
    }

    RHIImage* RHI_VK_FUNC_IMPL(RHIImage, Factory)::CreateFromFile(RHIDevice* device,
                                                                  RHICommandBuffer* cmd,
                                                                  const std::filesystem::path& path,
                                                                  bool isSRGB,
                                                                  bool useMipmap,
                                                                  RHIImageUsages usages,
                                                                  bool detached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!device || !cmd || !cmd->IsValid());

        const auto filePath = path.string();
        RHIImageDesc fileDesc{};
        fileDesc.depth = 1;
        fileDesc.arrayLayers = 1;
        fileDesc.usages = RHIImageUsageFlagBits::Sampled | usages;
        fileDesc.initialState = RHIImageResourceState::ShaderRead;
        if (useMipmap)
        {
            fileDesc.usages |= RHIImageUsageFlagBits::TransferSource | RHIImageUsageFlagBits::TransferDestination;
        }

        if (stbi_is_hdr(filePath.c_str()) == 1)
        {
            int width = 0;
            int height = 0;
            int channels = 0;
            float* pixels = stbi_loadf(filePath.c_str(), &width, &height, &channels, STBI_rgb);
            if (!pixels || width <= 0 || height <= 0)
            {
                if (pixels) { stbi_image_free(pixels); }

                HZ_CORE_ERROR("Failed to load HDR image '{}'", filePath);
                return nullptr;
            }

            fileDesc.width = static_cast<uint32_t>(width);
            fileDesc.height = static_cast<uint32_t>(height);
            fileDesc.mipLevels = useMipmap ? DeduceMipLevelCount(fileDesc.width, fileDesc.height) : 1;
            fileDesc.format = RHIFormat::RGB32SFloat;

            const size_t dataSize = width * height * GetBytesPerPixel(fileDesc.format);
            auto* image = CreateFromRawData(device, cmd, fileDesc, pixels, dataSize, detached);
            stbi_image_free(pixels);
            return image;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
        if (!pixels || width <= 0 || height <= 0)
        {
            if (pixels) { stbi_image_free(pixels); }

            HZ_CORE_ERROR("Failed to load image '{}'", filePath);
            return nullptr;
        }

        fileDesc.width = static_cast<uint32_t>(width);
        fileDesc.height = static_cast<uint32_t>(height);
        fileDesc.mipLevels = useMipmap ? DeduceMipLevelCount(fileDesc.width, fileDesc.height) : 1;
        fileDesc.format = DeduceNonHDRFormat(channels, isSRGB);
        if (fileDesc.format == RHIFormat::Undefined)
        {
            stbi_image_free(pixels);
            HZ_CORE_ERROR("Unsupported image channel count {} for '{}'", channels, filePath);
            return nullptr;
        }

        const size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height)
                                * static_cast<size_t>(GetBytesPerPixel(fileDesc.format));
        std::vector<stbi_uc> pixelData(pixels, pixels + dataSize);
        stbi_image_free(pixels);

        return CreateFromRawData(device, cmd, fileDesc, pixelData.data(), pixelData.size(), detached);
    }

    void RHI_VK_FUNC_IMPL(RHIImage, Release)()
    {
        if (!m_IsValid) { return; }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner && !m_IsDetached) { deviceOwner->UnregisterImage(this); }
    }

    void RHI_VK_FUNC_IMPL(RHIImage, ReleaseImmediate)()
    {
        if (!m_IsValid) { return; }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner && !m_IsDetached) { deviceOwner->UnregisterImage(this); }
    }

    void RHI_VK_FUNC_IMPL(RHIImage, ReleaseWithoutUnregister)()
    {
        for (const auto& view : m_Views)
        {
            if (view) { view->ReleaseWithoutUnregister(); }
        }
        m_Views.Clear();

        if (!m_IsSwapchainImage)
        {
            auto allocator = m_AllocatorOwner ? m_AllocatorOwner->GetHandle() : VK_NULL_HANDLE;
            const auto image = static_cast<VkImage>(m_Image);
            const auto allocation = m_Allocation;

            if (m_DeviceOwner)
            {
                m_DeviceOwner->EnqueueDeletion([allocator, image, allocation]() {
                    VulkanMemoryAllocator::DestroyImage(allocator, image, allocation);
                });
            }
            else { VulkanMemoryAllocator::DestroyImage(allocator, image, allocation); }
        }

        m_Image = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_AllocatorOwner = nullptr;
        m_IsSwapchainImage = false;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHIImage, ReleaseImmediateWithoutUnregister)()
    {
        for (const auto& view : m_Views)
        {
            if (view) { view->ReleaseImmediateWithoutUnregister(); }
        }
        m_Views.Clear();

        if (!m_IsSwapchainImage)
        {
            auto allocator = m_AllocatorOwner ? m_AllocatorOwner->GetHandle() : VK_NULL_HANDLE;
            const auto image = static_cast<VkImage>(m_Image);
            const auto allocation = m_Allocation;
            VulkanMemoryAllocator::DestroyImage(allocator, image, allocation);
        }

        m_Image = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_AllocatorOwner = nullptr;
        m_IsSwapchainImage = false;
        m_IsDetached = false;
    }

    RHIImageView* RHI_VK_FUNC_IMPL(RHIImage, CreateView)(const RHIImageViewDesc& desc, bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_IsValid || !m_DeviceOwner);

        return m_DeviceOwner->CreateImageView(this, desc, isDetached);
    }

    bool RHI_VK_FUNC_IMPL(RHIImage, Transition)(RHICommandBuffer* commandBuffer,
                                                RHIImageResourceState oldState,
                                                RHIImageResourceState newState)
    {
        RHIImageSubresourceRange fullRange;
        fullRange.levelCount = m_Desc.mipLevels;
        fullRange.layerCount = m_Desc.arrayLayers;
        fullRange.planes = GetDefaultTransitionPlanes(m_Desc.format);
        return Transition(commandBuffer, oldState, newState, fullRange);
    }

    bool RHI_VK_FUNC_IMPL(RHIImage, Transition)(RHICommandBuffer* commandBuffer,
                                                RHIImageResourceState oldState,
                                                RHIImageResourceState newState,
                                                const RHIImageSubresourceRange& subresourceRange,
                                                RHIQueue* srcQueue,
                                                RHIQueue* dstQueue)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !commandBuffer || !commandBuffer->IsValid());

        RHIPipelineBarrierDesc barrierDesc;
        barrierDesc.imageBarriers.push_back({this,
                                             GetPipelineStagesForState(oldState),
                                             GetPipelineStagesForState(newState),
                                             GetAccessFlagsForState(oldState),
                                             GetAccessFlagsForState(newState),
                                             oldState,
                                             newState,
                                             srcQueue,
                                             dstQueue,
                                             subresourceRange});

        m_CurrentState = newState;
        return commandBuffer->PipelineBarriers(barrierDesc);
    }

    void RHI_VK_FUNC_IMPL(RHIImage, RegisterView)(std::unique_ptr<RHIImageView> view)
    {
        m_Views.Register(std::move(view));
    }

    void RHI_VK_FUNC_IMPL(RHIImage, UnregisterView)(RHIImageView* view) { m_Views.Unregister(view); }
} // namespace Hazel