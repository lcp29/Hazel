//
// Created by helmholtz on 2026/3/13.
//

#include "RHIFactory.h"

#include "Vulkan/VulkanCommon.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanImage.h"
#include "Vulkan/VulkanMemoryAllocator.h"
#include "Vulkan/VulkanQueue.h"
#include "Vulkan/VulkanInstance.h"

#include <fstream>
#include <cstring>
#include <shaderc/shaderc.hpp>

namespace Hazel
{
    namespace
    {
        shaderc_shader_kind ToShadercShaderKind(const RHIShaderStageFlagBits stage)
        {
            switch (stage)
            {
                case RHIShaderStageFlagBits::Vertex:
                    return shaderc_glsl_vertex_shader;
                case RHIShaderStageFlagBits::Fragment:
                    return shaderc_glsl_fragment_shader;
                case RHIShaderStageFlagBits::Compute:
                    return shaderc_glsl_compute_shader;
            }

            return shaderc_glsl_infer_from_source;
        }

        std::string ReadTextFile(const std::filesystem::path &path)
        {
            std::ifstream input(path, std::ios::in | std::ios::binary);
            if (!input)
            {
                return {};
            }

            input.seekg(0, std::ios::end);
            const auto size = input.tellg();
            if (size <= 0)
            {
                return {};
            }

            std::string contents(static_cast<size_t>(size), '\0');
            input.seekg(0, std::ios::beg);
            input.read(contents.data(), static_cast<std::streamsize>(size));
            return contents;
        }

        RHIImageResourceState GetUploadFinalState(const RHIImageDesc &desc)
        {
            if (desc.initialState != RHIImageResourceState::Undefined)
            {
                return desc.initialState;
            }

            if (desc.usages & RHIImageUsageFlagBits::Sampled)
            {
                return RHIImageResourceState::ShaderRead;
            }
            if (desc.usages & RHIImageUsageFlagBits::ColorAttachment)
            {
                return RHIImageResourceState::ColorAttachment;
            }
            if (desc.usages & RHIImageUsageFlagBits::DepthStencilAttachment)
            {
                return RHIImageResourceState::DepthStencilAttachment;
            }

            return RHIImageResourceState::TransferDestination;
        }

        RHIPipelineStages GetStageMask(RHIImageResourceState state)
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
                    return RHIPipelineStageFlagBits::AllCommands;
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

        vk::AccessFlags2 GetAccessMask(RHIImageResourceState state)
        {
            switch (state)
            {
                case RHIImageResourceState::Undefined:
                    return {};
                case RHIImageResourceState::Common:
                    return vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
                case RHIImageResourceState::TransferSource:
                    return vk::AccessFlagBits2::eTransferRead;
                case RHIImageResourceState::TransferDestination:
                    return vk::AccessFlagBits2::eTransferWrite;
                case RHIImageResourceState::ShaderRead:
                    return vk::AccessFlagBits2::eShaderRead;
                case RHIImageResourceState::ShaderWrite:
                    return vk::AccessFlagBits2::eShaderWrite;
                case RHIImageResourceState::ColorAttachment:
                    return vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
                case RHIImageResourceState::DepthStencilAttachment:
                    return vk::AccessFlagBits2::eDepthStencilAttachmentRead
                           | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                case RHIImageResourceState::Present:
                    return {};
            }

            return {};
        }

        vk::ImageAspectFlags GetAspectMask(RHIFormat format)
        {
            switch (format)
            {
                case RHIFormat::D32SFloat:
                    return vk::ImageAspectFlagBits::eDepth;
                case RHIFormat::D32SFloatS8Uint:
                    return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
                default:
                    return vk::ImageAspectFlagBits::eColor;
            }
        }
    }

    std::optional<std::unique_ptr<RHIInstance>> CreateInstance(const RHIInstanceDesc &desc)
    {
        switch (desc.backend)
        {
            case RHIBackend::Auto:
            case RHIBackend::Vulkan:
            {
                auto instance = std::make_unique<RHIInstance>(desc);
                return instance->IsValid() ? std::make_optional(std::move(instance)) : std::nullopt;
            }
        }
        return std::nullopt;
    }

    RHIImage *CreateImageFromLinearBuffer(RHIDevice &device,
                                          const RHIImageDesc &desc,
                                          const void *data,
                                          size_t dataSize,
                                          RHIQueue *queue)
    {
        if (!data || dataSize == 0)
        {
            return nullptr;
        }

        auto *vkDevice = &device;
        auto allocator = vkDevice->GetAllocator();
        if (!allocator || !allocator->IsValid())
        {
            return nullptr;
        }

        RHIImageDesc imageDesc = desc;
        imageDesc.usages = imageDesc.usages | RHIImageUsageFlagBits::TransferDestination;
        auto image = vkDevice->CreateImage(imageDesc);
        if (!image || !image->IsValid())
        {
            return nullptr;
        }

        auto *vkImage = image;

        VkBufferCreateInfo stagingBufferCreateInfo{};
        stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferCreateInfo.size = dataSize;
        stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocationCreateInfo{};
        stagingAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocationCreateInfo.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VmaAllocation stagingAllocation = VK_NULL_HANDLE;
        if (!allocator->CreateBuffer(
                stagingBufferCreateInfo,
                stagingAllocationCreateInfo,
                &stagingBuffer,
                &stagingAllocation))
        {
            return nullptr;
        }

        auto mappedData = allocator->MapMemory(stagingAllocation);
        if (!mappedData)
        {
            VulkanMemoryAllocator::DestroyBuffer(allocator->GetHandle(), stagingBuffer, stagingAllocation);
            return nullptr;
        }

        std::memcpy(mappedData, data, dataSize);
        allocator->UnmapMemory(stagingAllocation);

        RHIQueue *selectedQueue = nullptr;
        if (queue)
        {
            if (queue->GetType() != RHIQueueType::Transfer && queue->GetType() != RHIQueueType::Graphics)
            {
                VulkanMemoryAllocator::DestroyBuffer(allocator->GetHandle(), stagingBuffer, stagingAllocation);
                return nullptr;
            }

            selectedQueue = vkDevice->GetQueue(queue->GetType());
            if (!selectedQueue || selectedQueue != queue)
            {
                VulkanMemoryAllocator::DestroyBuffer(allocator->GetHandle(), stagingBuffer, stagingAllocation);
                return nullptr;
            }
        }
        else
        {
            const auto transferQueue = vkDevice->GetQueue(RHIQueueType::Transfer);
            const auto graphicsQueue = vkDevice->GetQueue(RHIQueueType::Graphics);
            selectedQueue = transferQueue ? transferQueue : graphicsQueue;
        }

        if (!selectedQueue)
        {
            VulkanMemoryAllocator::DestroyBuffer(allocator->GetHandle(), stagingBuffer, stagingAllocation);
            return nullptr;
        }

        vk::CommandPoolCreateInfo poolCreateInfo;
        poolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
        poolCreateInfo.queueFamilyIndex = selectedQueue->GetFamilyIndex();

        auto handle = vkDevice->GetHandle();
        auto commandPool = handle.createCommandPool(poolCreateInfo);

        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;
        auto commandBuffers = handle.allocateCommandBuffers(allocInfo);
        auto commandBuffer = commandBuffers.front();

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        commandBuffer.begin(beginInfo);

        vk::ImageMemoryBarrier2 toTransferBarrier;
        toTransferBarrier.srcStageMask = VulkanConvertPipelineStages(GetStageMask(RHIImageResourceState::Undefined));
        toTransferBarrier.srcAccessMask = GetAccessMask(RHIImageResourceState::Undefined);
        toTransferBarrier.dstStageMask = VulkanConvertPipelineStages(GetStageMask(RHIImageResourceState::TransferDestination));
        toTransferBarrier.dstAccessMask = GetAccessMask(RHIImageResourceState::TransferDestination);
        toTransferBarrier.oldLayout = VulkanConvertResourceState(RHIImageResourceState::Undefined);
        toTransferBarrier.newLayout = VulkanConvertResourceState(RHIImageResourceState::TransferDestination);
        toTransferBarrier.image = vkImage->GetHandle();
        toTransferBarrier.subresourceRange.aspectMask = GetAspectMask(imageDesc.format);
        toTransferBarrier.subresourceRange.baseMipLevel = 0;
        toTransferBarrier.subresourceRange.levelCount = imageDesc.mipLevels;
        toTransferBarrier.subresourceRange.baseArrayLayer = 0;
        toTransferBarrier.subresourceRange.layerCount = imageDesc.arrayLayers;

        vk::DependencyInfo toTransferDependency;
        toTransferDependency.imageMemoryBarrierCount = 1;
        toTransferDependency.pImageMemoryBarriers = &toTransferBarrier;
        commandBuffer.pipelineBarrier2(toTransferDependency);

        vk::BufferImageCopy copyRegion;
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = GetAspectMask(imageDesc.format);
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = imageDesc.arrayLayers;
        copyRegion.imageOffset = vk::Offset3D(0, 0, 0);
        copyRegion.imageExtent = vk::Extent3D(imageDesc.width, imageDesc.height, imageDesc.depth);

        commandBuffer.copyBufferToImage(
            stagingBuffer,
            vkImage->GetHandle(),
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &copyRegion);

        const auto finalState = GetUploadFinalState(imageDesc);
        if (finalState != RHIImageResourceState::TransferDestination)
        {
            vk::ImageMemoryBarrier2 toFinalBarrier;
            toFinalBarrier.srcStageMask = VulkanConvertPipelineStages(GetStageMask(RHIImageResourceState::TransferDestination));
            toFinalBarrier.srcAccessMask = GetAccessMask(RHIImageResourceState::TransferDestination);
            toFinalBarrier.dstStageMask = VulkanConvertPipelineStages(GetStageMask(finalState));
            toFinalBarrier.dstAccessMask = GetAccessMask(finalState);
            toFinalBarrier.oldLayout = VulkanConvertResourceState(RHIImageResourceState::TransferDestination);
            toFinalBarrier.newLayout = VulkanConvertResourceState(finalState);
            toFinalBarrier.image = vkImage->GetHandle();
            toFinalBarrier.subresourceRange = toTransferBarrier.subresourceRange;

            vk::DependencyInfo toFinalDependency;
            toFinalDependency.imageMemoryBarrierCount = 1;
            toFinalDependency.pImageMemoryBarriers = &toFinalBarrier;
            commandBuffer.pipelineBarrier2(toFinalDependency);
        }

        commandBuffer.end();

        auto vkQueue = selectedQueue->GetHandle();
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueue.submit(submitInfo, {});
        vkQueue.waitIdle();

        handle.freeCommandBuffers(commandPool, commandBuffers);
        handle.destroyCommandPool(commandPool);
        VulkanMemoryAllocator::DestroyBuffer(allocator->GetHandle(), stagingBuffer, stagingAllocation);

        return image;
    }

    RHIShader *CreateShaderFromGLSLFile(RHIDevice &device, const RHIShaderFileDesc &desc)
    {
        if (desc.path.empty())
        {
            return nullptr;
        }

        const auto source = ReadTextFile(desc.path);
        if (source.empty())
        {
            return nullptr;
        }

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetSourceLanguage(shaderc_source_language_glsl);

        auto result = compiler.CompileGlslToSpv(
            source,
            ToShadercShaderKind(desc.stage),
            desc.path.string().c_str(),
            desc.entryPoint.c_str(),
            options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            return nullptr;
        }

        RHIShaderDesc shaderDesc;
        shaderDesc.stage = desc.stage;
        shaderDesc.entryPoint = desc.entryPoint;
        shaderDesc.debugName = desc.debugName.empty() ? desc.path.filename().string() : desc.debugName;
        shaderDesc.binary.assign(result.cbegin(), result.cend());

        return device.CreateShader(shaderDesc);
    }
} // Hazel
