//
// Created by helmholtz on 2026/3/15.
//

#define VULKAN_HPP_NO_EXCEPTIONS

#include "VulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanCommandPool.h"
#include "VulkanCommon.h"
#include "VulkanComputePipeline.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanPipelineCommon.h"
#include "VulkanQueue.h"
#include "VulkanResourceGroup.h"
#include "VulkanResourceSignature.h"

#include <array>
#include <concepts>

namespace Hazel
{
    namespace
    {
        constexpr uint32_t s_DrawIndirectCommandSize = sizeof(VkDrawIndirectCommand);
        constexpr uint32_t s_DrawIndexedIndirectCommandSize = sizeof(VkDrawIndexedIndirectCommand);

        vk::CommandBufferLevel VulkanConvertCommandBufferLevel(RHICommandBufferLevel level)
        {
            switch (level)
            {
                case RHICommandBufferLevel::Primary:
                    return vk::CommandBufferLevel::ePrimary;
                case RHICommandBufferLevel::Secondary:
                    return vk::CommandBufferLevel::eSecondary;
            }

            return vk::CommandBufferLevel::ePrimary;
        }

        vk::IndexType VulkanConvertIndexType(RHIIndexType type)
        {
            switch (type)
            {
                case RHIIndexType::UInt16:
                    return vk::IndexType::eUint16;
                case RHIIndexType::UInt32:
                    return vk::IndexType::eUint32;
            }

            return vk::IndexType::eUint32;
        }

        vk::AttachmentLoadOp VulkanConvertRenderingLoadOp(RHIRenderingLoadOp loadOp)
        {
            switch (loadOp)
            {
                case RHIRenderingLoadOp::Load:
                    return vk::AttachmentLoadOp::eLoad;
                case RHIRenderingLoadOp::Clear:
                    return vk::AttachmentLoadOp::eClear;
                case RHIRenderingLoadOp::DontCare:
                    return vk::AttachmentLoadOp::eDontCare;
            }

            return vk::AttachmentLoadOp::eDontCare;
        }

        vk::AttachmentStoreOp VulkanConvertRenderingStoreOp(RHIRenderingStoreOp storeOp)
        {
            switch (storeOp)
            {
                case RHIRenderingStoreOp::Store:
                    return vk::AttachmentStoreOp::eStore;
                case RHIRenderingStoreOp::DontCare:
                    return vk::AttachmentStoreOp::eDontCare;
            }

            return vk::AttachmentStoreOp::eDontCare;
        }

        vk::Filter VulkanConvertBlitFilter(RHIBlitFilter filter)
        {
            switch (filter)
            {
                case RHIBlitFilter::Nearest:
                    return vk::Filter::eNearest;
                case RHIBlitFilter::Linear:
                    return vk::Filter::eLinear;
            }

            return vk::Filter::eLinear;
        }

        vk::ClearColorValue VulkanConvertClearColorValue(const RHIClearColorValue& value)
        {
            switch (value.type)
            {
                case RHIClearColorValue::Type::Float:
                    return vk::ClearColorValue(value.float32);
                case RHIClearColorValue::Type::Int:
                    return vk::ClearColorValue(value.int32);
                case RHIClearColorValue::Type::UInt:
                    return vk::ClearColorValue(value.uint32);
            }
            return {};
        }

        vk::ClearDepthStencilValue VulkanConvertClearDepthStencilValue(const RHIClearDepthStencilValue& value)
        {
            return vk::ClearDepthStencilValue(value.depth, value.stencil);
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHICommandBuffer, RHICommandBufferImpl)(RHICommandPool* commandPoolOwner,
                                                             vk::Device device,
                                                             vk::CommandPool commandPool,
                                                             const RHICommandBufferDesc& desc)
    {
        m_CommandPoolOwner = commandPoolOwner;
        m_Device = device;
        m_CommandPool = commandPool;
        m_Desc = desc;

        if (!m_CommandPoolOwner || !m_Device || !m_CommandPool)
        {
            return;
        }

        vk::CommandBufferAllocateInfo allocateInfo;
        allocateInfo.commandPool = m_CommandPool;
        allocateInfo.level = VulkanConvertCommandBufferLevel(desc.level);
        allocateInfo.commandBufferCount = 1;

        const auto commandBuffers = m_Device.allocateCommandBuffers(allocateInfo);
        if (commandBuffers.value.empty())
        {
            return;
        }

        m_CommandBuffer = commandBuffers.value.front();
        m_IsValid = static_cast<bool>(m_CommandBuffer);
    }

    RHI_VK_FUNC_IMPL(RHICommandBuffer, ~RHICommandBufferImpl)()
    {
        Release();
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, Begin)(bool oneTimeSubmit)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || m_IsRecording);

        vk::CommandBufferBeginInfo beginInfo;
        if (oneTimeSubmit)
        {
            beginInfo.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        }
        if (m_CommandBuffer.begin(beginInfo) != vk::Result::eSuccess)
        {
            return false;
        }

        m_IsRendering = false;
        m_IsRecording = true;
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, End)()
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering);

        if (m_CommandBuffer.end() != vk::Result::eSuccess)
        {
            return false;
        }

        m_IsRecording = false;
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, Reset)()
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || m_IsRecording || m_IsRendering);

        if (m_CommandBuffer.reset() != vk::Result::eSuccess)
        {
            return false;
        }

        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BeginRendering)(

        const RHIRenderingInfo& info
    )
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering);

        std::vector<vk::RenderingAttachmentInfo> colorAttachments;
        colorAttachments.reserve(info.colorAttachments.size());
        for (const auto& attachment : info.colorAttachments)
        {
            auto* imageView = attachment.imageView;
            HZ_RHI_DEBUG_FAIL_IF(!imageView || !imageView->IsValid());

            vk::RenderingAttachmentInfo attachmentInfo;
            attachmentInfo.imageView = imageView->GetHandle();
            attachmentInfo.imageLayout = VulkanConvertImageResourceState(attachment.state);
            attachmentInfo.loadOp = VulkanConvertRenderingLoadOp(attachment.loadOp);
            attachmentInfo.storeOp = VulkanConvertRenderingStoreOp(attachment.storeOp);
            attachmentInfo.clearValue.color = VulkanConvertClearColorValue(attachment.clearColorValue);
            colorAttachments.push_back(attachmentInfo);
        }

        std::optional<vk::RenderingAttachmentInfo> depthAttachment;
        std::optional<vk::RenderingAttachmentInfo> stencilAttachment;

        if (info.depthAttachment.has_value())
        {
            const auto& attachment = info.depthAttachment.value();
            auto* imageView = attachment.imageView;
            HZ_RHI_DEBUG_FAIL_IF(!imageView || !imageView->IsValid());

            depthAttachment.emplace();
            depthAttachment->imageView = imageView->GetHandle();
            depthAttachment->imageLayout = VulkanConvertImageResourceState(attachment.state);
            depthAttachment->loadOp = VulkanConvertRenderingLoadOp(attachment.loadOp);
            depthAttachment->storeOp = VulkanConvertRenderingStoreOp(attachment.storeOp);
            depthAttachment->clearValue.depthStencil.depth = attachment.clearDepthStencilValue.depth;
        }

        if (info.stencilAttachment.has_value())
        {
            const auto& attachment = info.stencilAttachment.value();
            auto* imageView = attachment.imageView;
            HZ_RHI_DEBUG_FAIL_IF(!imageView || !imageView->IsValid());

            stencilAttachment.emplace();
            stencilAttachment->imageView = imageView->GetHandle();
            stencilAttachment->imageLayout = VulkanConvertImageResourceState(attachment.state);
            stencilAttachment->loadOp = VulkanConvertRenderingLoadOp(attachment.loadOp);
            stencilAttachment->storeOp = VulkanConvertRenderingStoreOp(attachment.storeOp);
            stencilAttachment->clearValue.depthStencil.stencil = attachment.clearDepthStencilValue.stencil;
        }

        vk::RenderingInfo renderingInfo;
        renderingInfo.renderArea = vk::Rect2D(vk::Offset2D(info.renderOffset.x, info.renderOffset.y),
                                              vk::Extent2D(info.renderViewSize.width, info.renderViewSize.height));
        renderingInfo.layerCount = info.layerCount;
        renderingInfo.viewMask = info.viewMask;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
        renderingInfo.pColorAttachments = colorAttachments.data();
        if (depthAttachment.has_value())
        {
            renderingInfo.pDepthAttachment = &depthAttachment.value();
        }
        if (stencilAttachment.has_value())
        {
            renderingInfo.pStencilAttachment = &stencilAttachment.value();
        }

        m_CommandBuffer.beginRendering(renderingInfo);
        m_IsRendering = true;
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, EndRendering)()
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !m_IsRendering);

        m_CommandBuffer.endRendering();
        m_IsRendering = false;
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindGraphicsPipeline)(RHIGraphicsPipeline* pipeline)
    {
        auto* vkPipeline = pipeline;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkPipeline || !vkPipeline->IsValid());

        m_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipeline->GetHandle());
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindComputePipeline)(RHIComputePipeline* pipeline)
    {
        auto* vkPipeline = pipeline;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkPipeline || !vkPipeline->IsValid());

        m_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, vkPipeline->GetHandle());
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindVertexBuffer)(uint32_t binding,
                                                              RHIBuffer* buffer,
                                                              uint64_t offset)
    {
        auto* vkBuffer = buffer;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkBuffer || !vkBuffer->IsValid());

        const vk::Buffer handle = vkBuffer->GetHandle();
        const vk::DeviceSize deviceOffset = offset;
        m_CommandBuffer.bindVertexBuffers(binding, 1, &handle, &deviceOffset);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindIndexBuffer)(RHIBuffer* buffer,
                                                             RHIIndexType indexType,
                                                             uint64_t offset)
    {
        auto* vkBuffer = buffer;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkBuffer || !vkBuffer->IsValid());

        m_CommandBuffer.bindIndexBuffer(vkBuffer->GetHandle(), offset, VulkanConvertIndexType(indexType));
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindGraphicsResourceGroup)(RHIGraphicsPipeline* pipeline,
                                                                       uint32_t set,
                                                                       RHIResourceGroup* resourceGroup)
    {
        auto* vkPipeline = pipeline;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkPipeline || !vkPipeline->IsValid());

        return BindGraphicsResourceGroup(vkPipeline->GetDesc().resourceSignature, set, resourceGroup);
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindGraphicsResourceGroup)(RHIResourceSignature* signature,
                                                                       uint32_t set,
                                                                       RHIResourceGroup* resourceGroup)
    {
        auto* vkSignature = signature;
        auto* vkGroup = resourceGroup;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkSignature || !vkGroup || !vkSignature->IsValid()
            || !vkGroup->IsValid());

        const vk::DescriptorSet descriptorSet = vkGroup->GetHandle();
        m_CommandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            vkSignature->GetPipelineLayout(),
            set,
            1,
            &descriptorSet,
            0,
            nullptr);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindComputeResourceGroup)(RHIComputePipeline* pipeline,
                                                                      uint32_t set,
                                                                      RHIResourceGroup* resourceGroup)
    {
        auto* vkPipeline = pipeline;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkPipeline || !vkPipeline->IsValid());

        return BindComputeResourceGroup(vkPipeline->GetDesc().resourceSignature, set, resourceGroup);
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindComputeResourceGroup)(RHIResourceSignature* signature,
                                                                      uint32_t set,
                                                                      RHIResourceGroup* resourceGroup)
    {
        auto* vkSignature = signature;
        auto* vkGroup = resourceGroup;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkSignature || !vkGroup || !vkSignature->IsValid()
            || !vkGroup->IsValid());

        const vk::DescriptorSet descriptorSet = vkGroup->GetHandle();
        m_CommandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            vkSignature->GetPipelineLayout(),
            set,
            1,
            &descriptorSet,
            0,
            nullptr);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BlitImage)(RHIImage* srcImage,
                                                       RHIImageResourceState srcState,
                                                       RHIImage* dstImage,
                                                       RHIImageResourceState dstState,
                                                       const RHIImageBlitDesc& desc)
    {
        auto* vkSrcImage = srcImage;
        auto* vkDstImage = dstImage;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering || !vkSrcImage || !vkDstImage
            || !vkSrcImage->IsValid() || !vkDstImage->IsValid());

        std::vector<vk::ImageBlit> blitRegions;
        blitRegions.reserve(desc.regions.size());
        for (const auto& region : desc.regions)
        {
            const auto& srcSubresource = region.srcSubresource;
            const auto& dstSubresource = region.dstSubresource;
            vk::ImageBlit blitRegion;
            blitRegion.srcSubresource.aspectMask = VulkanConvertImagePlanes(srcSubresource.planes);
            blitRegion.srcSubresource.mipLevel = srcSubresource.mipLevel;
            blitRegion.srcSubresource.baseArrayLayer = srcSubresource.baseArrayLayer;
            blitRegion.srcSubresource.layerCount = srcSubresource.layerCount;
            blitRegion.srcOffsets[0] =
                vk::Offset3D(region.srcOffsets[0].x, region.srcOffsets[0].y, region.srcOffsets[0].z);
            blitRegion.srcOffsets[1] =
                vk::Offset3D(region.srcOffsets[1].x, region.srcOffsets[1].y, region.srcOffsets[1].z);
            blitRegion.dstSubresource.aspectMask = VulkanConvertImagePlanes(dstSubresource.planes);
            blitRegion.dstSubresource.mipLevel = dstSubresource.mipLevel;
            blitRegion.dstSubresource.baseArrayLayer = dstSubresource.baseArrayLayer;
            blitRegion.dstSubresource.layerCount = dstSubresource.layerCount;
            blitRegion.dstOffsets[0] =
                vk::Offset3D(region.dstOffsets[0].x, region.dstOffsets[0].y, region.dstOffsets[0].z);
            blitRegion.dstOffsets[1] =
                vk::Offset3D(region.dstOffsets[1].x, region.dstOffsets[1].y, region.dstOffsets[1].z);
            blitRegions.push_back(blitRegion);
        }

        m_CommandBuffer.blitImage(vkSrcImage->GetHandle(),
                                  VulkanConvertImageResourceState(srcState),
                                  vkDstImage->GetHandle(),
                                  VulkanConvertImageResourceState(dstState),
                                  blitRegions,
                                  VulkanConvertBlitFilter(desc.filter));
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, CopyBufferToImage)(RHIBuffer* srcBuffer,
                                                               uint64_t bufferOffset,
                                                               RHIExtent2D bufferMemoryExtent,
                                                               RHIImage* dstImage,
                                                               RHIOffset3D dstOffset,
                                                               RHIExtent3D dstExtent,
                                                               RHIImageSubresourceLayers dstSubresource)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !srcBuffer || !dstImage || !srcBuffer->IsValid()
            || !dstImage->IsValid());
        vk::BufferImageCopy2 info{};
        info.bufferOffset = bufferOffset;
        info.bufferRowLength = bufferMemoryExtent.width;
        info.bufferImageHeight = bufferMemoryExtent.height;
        info.imageSubresource.aspectMask = VulkanConvertImagePlanes(dstSubresource.planes);
        info.imageSubresource.mipLevel = dstSubresource.mipLevel;
        info.imageSubresource.baseArrayLayer = dstSubresource.baseArrayLayer;
        info.imageSubresource.layerCount = dstSubresource.layerCount;
        info.imageOffset = vk::Offset3D(dstOffset.x, dstOffset.y, dstOffset.z);
        info.imageExtent = vk::Extent3D(dstExtent.width, dstExtent.height, dstExtent.depth);

        vk::CopyBufferToImageInfo2 copyInfo{};
        copyInfo.srcBuffer = srcBuffer->GetHandle();
        copyInfo.dstImage = dstImage->GetHandle();
        copyInfo.dstImageLayout = VulkanConvertImageResourceState(dstImage->GetCurrentState());
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &info;

        m_CommandBuffer.copyBufferToImage2(&copyInfo);

        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, CopyImageToBuffer)(RHIImage* srcImage,
                                                               RHIBuffer* dstBuffer,
                                                               uint64_t bufferOffset,
                                                               RHIExtent2D bufferMemoryExtent,
                                                               RHIOffset3D srcOffset,
                                                               RHIExtent3D srcExtent,
                                                               RHIImageSubresourceLayers srcSubresource)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering || !srcImage || !dstBuffer
            || !srcImage->IsValid() || !dstBuffer->IsValid());

        vk::BufferImageCopy2 info{};
        info.bufferOffset = bufferOffset;
        info.bufferRowLength = bufferMemoryExtent.width;
        info.bufferImageHeight = bufferMemoryExtent.height;
        info.imageSubresource.aspectMask = VulkanConvertImagePlanes(srcSubresource.planes);
        info.imageSubresource.mipLevel = srcSubresource.mipLevel;
        info.imageSubresource.baseArrayLayer = srcSubresource.baseArrayLayer;
        info.imageSubresource.layerCount = srcSubresource.layerCount;
        info.imageOffset = vk::Offset3D(srcOffset.x, srcOffset.y, srcOffset.z);
        info.imageExtent = vk::Extent3D(srcExtent.width, srcExtent.height, srcExtent.depth);

        vk::CopyImageToBufferInfo2 copyInfo{};
        copyInfo.srcImage = srcImage->GetHandle();
        copyInfo.srcImageLayout = VulkanConvertImageResourceState(srcImage->GetCurrentState());
        copyInfo.dstBuffer = dstBuffer->GetHandle();
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &info;

        m_CommandBuffer.copyImageToBuffer2(&copyInfo);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, ClearAttachments)(const RHIClearAttachmentsDesc& desc)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !m_IsRendering);

        std::vector<vk::ClearAttachment> attachments;
        attachments.reserve(desc.attachments.size());
        for (const auto& attachment : desc.attachments)
        {
            vk::ClearAttachment clearAttachment{};
            clearAttachment.aspectMask = VulkanConvertImagePlanes(attachment.planes);
            clearAttachment.colorAttachment = attachment.colorAttachment;
            if ((attachment.planes & RHIImagePlaneFlagBits::Color) != RHIImagePlanes())
            {
                clearAttachment.clearValue.color = VulkanConvertClearColorValue(attachment.colorValue);
            }
            else
            {
                clearAttachment.clearValue.depthStencil =
                    VulkanConvertClearDepthStencilValue(attachment.depthStencilValue);
            }

            attachments.push_back(clearAttachment);
        }

        std::vector<vk::ClearRect> rects;
        rects.reserve(desc.rects.size());
        for (const auto& rect : desc.rects)
        {
            vk::ClearRect clearRect{};
            clearRect.rect = vk::Rect2D(vk::Offset2D(rect.offset.x, rect.offset.y),
                                        vk::Extent2D(rect.extent.width, rect.extent.height));
            clearRect.baseArrayLayer = rect.baseArrayLayer;
            clearRect.layerCount = rect.layerCount;
            rects.push_back(clearRect);
        }

        m_CommandBuffer.clearAttachments(attachments, rects);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, ClearColorImage)(RHIImage* image,
                                                             RHIImageResourceState state,
                                                             const RHIClearColorValue& value,
                                                             const RHIImageSubresourceRange& subresourceRange)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering || !image || !image->IsValid());

        vk::ImageSubresourceRange range{};
        range.aspectMask = VulkanConvertImagePlanes(subresourceRange.planes);
        range.baseMipLevel = subresourceRange.baseMipLevel;
        range.levelCount = subresourceRange.levelCount;
        range.baseArrayLayer = subresourceRange.baseArrayLayer;
        range.layerCount = subresourceRange.layerCount;

        const vk::ClearColorValue clearValue = VulkanConvertClearColorValue(value);
        m_CommandBuffer.clearColorImage(image->GetHandle(), VulkanConvertImageResourceState(state), clearValue, range);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, ClearDepthStencilImage)(RHIImage* image,
                                                                    RHIImageResourceState state,
                                                                    const RHIClearDepthStencilValue& value,
                                                                    const RHIImageSubresourceRange& subresourceRange)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering || !image || !image->IsValid());

        vk::ImageSubresourceRange range{};
        range.aspectMask = VulkanConvertImagePlanes(subresourceRange.planes);
        range.baseMipLevel = subresourceRange.baseMipLevel;
        range.levelCount = subresourceRange.levelCount;
        range.baseArrayLayer = subresourceRange.baseArrayLayer;
        range.layerCount = subresourceRange.layerCount;

        const vk::ClearDepthStencilValue clearValue = VulkanConvertClearDepthStencilValue(value);
        m_CommandBuffer.clearDepthStencilImage(
            image->GetHandle(),
            VulkanConvertImageResourceState(state),
            clearValue,
            range);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, CopyBuffer)(RHIBuffer* srcBuffer,
                                                        RHIBuffer* dstBuffer,
                                                        const RHIBufferCopyDesc& desc)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering || !srcBuffer || !dstBuffer
            || !srcBuffer->IsValid() || !dstBuffer->IsValid());

        std::vector<vk::BufferCopy2> regions;
        regions.reserve(desc.regions.size());
        for (const auto& region : desc.regions)
        {
            vk::BufferCopy2 copyRegion{};
            copyRegion.srcOffset = region.srcOffset;
            copyRegion.dstOffset = region.dstOffset;
            copyRegion.size = region.size;
            regions.push_back(copyRegion);
        }

        vk::CopyBufferInfo2 copyInfo{};
        copyInfo.srcBuffer = srcBuffer->GetHandle();
        copyInfo.dstBuffer = dstBuffer->GetHandle();
        copyInfo.regionCount = static_cast<uint32_t>(regions.size());
        copyInfo.pRegions = regions.data();

        m_CommandBuffer.copyBuffer2(&copyInfo);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, Draw)(uint32_t vertexCount,
                                                  uint32_t instanceCount,
                                                  uint32_t firstVertex,
                                                  uint32_t firstInstance)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !m_IsRendering);

        m_CommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, DrawIndexed)(uint32_t indexCount,
                                                         uint32_t instanceCount,
                                                         uint32_t firstIndex,
                                                         int32_t vertexOffset,
                                                         uint32_t firstInstance)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !m_IsRendering);

        m_CommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, DrawIndirect)(RHIBuffer* buffer,
                                                          uint64_t offset,
                                                          uint32_t drawCount,
                                                          uint32_t stride)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !m_IsRendering || !buffer || !buffer->IsValid());

        const uint32_t resolvedStride = stride == 0 ? s_DrawIndirectCommandSize : stride;

        m_CommandBuffer.drawIndirect(buffer->GetHandle(), offset, drawCount, resolvedStride);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, DrawIndexedIndirect)(RHIBuffer* buffer,
                                                                 uint64_t offset,
                                                                 uint32_t drawCount,
                                                                 uint32_t stride)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !m_IsRendering || !buffer || !buffer->IsValid());

        const uint32_t resolvedStride = stride == 0 ? s_DrawIndexedIndirectCommandSize : stride;

        m_CommandBuffer.drawIndexedIndirect(buffer->GetHandle(), offset, drawCount, resolvedStride);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, Dispatch)(uint32_t groupCountX,
                                                      uint32_t groupCountY,
                                                      uint32_t groupCountZ)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering);

        m_CommandBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, DispatchIndirect)(RHIBuffer* buffer, uint64_t offset)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || m_IsRendering || !buffer || !buffer->IsValid());

        m_CommandBuffer.dispatchIndirect(buffer->GetHandle(), offset);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, SetViewport)(float x,
                                                         float y,
                                                         float width,
                                                         float height,
                                                         float minDepth,
                                                         float maxDepth)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording);

        const vk::Viewport viewport(x, y, width, height, minDepth, maxDepth);
        m_CommandBuffer.setViewport(0, viewport);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, SetScissor)(int32_t x,
                                                        int32_t y,
                                                        uint32_t width,
                                                        uint32_t height)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording);

        const vk::Rect2D scissor({x, y}, {width, height});
        m_CommandBuffer.setScissor(0, scissor);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, SetBlendConstants)(float red,
                                                               float green,
                                                               float blue,
                                                               float alpha)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording);

        const float blendConstants[] = {red, green, blue, alpha};
        m_CommandBuffer.setBlendConstants(blendConstants);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, SetStencilReference)(uint32_t reference)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording);

        m_CommandBuffer.setStencilReference(vk::StencilFaceFlagBits::eFront | vk::StencilFaceFlagBits::eBack,
                                            reference);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, PushConstants)(RHIResourceSignature* signature,
                                                           const RHIShaderStages& stages,
                                                           uint32_t offset,
                                                           uint32_t size,
                                                           const void* data)
    {
        auto* vkSignature = signature;
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !m_IsRecording || !vkSignature || !vkSignature->IsValid() || !data);

        m_CommandBuffer.pushConstants(
            vkSignature->GetPipelineLayout(),
            VulkanConvertShaderStages(stages),
            offset,
            size,
            data);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, PipelineBarriers)(const RHIPipelineBarrierDesc& desc)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsRecording);
        vk::DependencyInfo dependencyInfo;

        dependencyInfo.memoryBarrierCount = desc.memoryBarriers.size();
        std::vector<vk::MemoryBarrier2> memoryBarriers;
        memoryBarriers.reserve(dependencyInfo.memoryBarrierCount);
        for (const auto& barrier : desc.memoryBarriers)
        {
            vk::MemoryBarrier2 memoryBarrier;
            memoryBarrier.srcAccessMask = VulkanConvertAccessFlags(barrier.srcAccess);
            memoryBarrier.dstAccessMask = VulkanConvertAccessFlags(barrier.dstAccess);
            memoryBarrier.srcStageMask = VulkanConvertPipelineStages(barrier.srcStages);
            memoryBarrier.dstStageMask = VulkanConvertPipelineStages(barrier.dstStages);
            memoryBarriers.push_back(memoryBarrier);
        }
        dependencyInfo.pMemoryBarriers = memoryBarriers.data();

        dependencyInfo.bufferMemoryBarrierCount = desc.bufferBarriers.size();
        std::vector<vk::BufferMemoryBarrier2> bufferMemoryBarriers;
        bufferMemoryBarriers.reserve(dependencyInfo.bufferMemoryBarrierCount);
        for (const auto& barrier : desc.bufferBarriers)
        {
            HZ_RHI_DEBUG_FAIL_IF(!barrier.buffer);
            vk::BufferMemoryBarrier2 bufferMemoryBarrier;
            bufferMemoryBarrier.srcAccessMask = VulkanConvertAccessFlags(barrier.srcAccess);
            bufferMemoryBarrier.dstAccessMask = VulkanConvertAccessFlags(barrier.dstAccess);
            bufferMemoryBarrier.srcStageMask = VulkanConvertPipelineStages(barrier.srcStages);
            bufferMemoryBarrier.dstStageMask = VulkanConvertPipelineStages(barrier.dstStages);
            bufferMemoryBarrier.srcQueueFamilyIndex =
                barrier.srcQueue ? barrier.srcQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.dstQueueFamilyIndex =
                barrier.dstQueue ? barrier.dstQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.buffer = barrier.buffer->GetHandle();
            bufferMemoryBarrier.offset = barrier.offset;
            bufferMemoryBarrier.size = barrier.size;
            bufferMemoryBarriers.push_back(bufferMemoryBarrier);
        }
        dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();

        dependencyInfo.imageMemoryBarrierCount = desc.imageBarriers.size();
        std::vector<vk::ImageMemoryBarrier2> imageMemoryBarriers;
        imageMemoryBarriers.reserve(dependencyInfo.imageMemoryBarrierCount);
        for (const auto& barrier : desc.imageBarriers)
        {
            HZ_RHI_DEBUG_FAIL_IF(!barrier.image);
            vk::ImageMemoryBarrier2 imageMemoryBarrier;
            imageMemoryBarrier.srcAccessMask = VulkanConvertAccessFlags(barrier.srcAccess);
            imageMemoryBarrier.dstAccessMask = VulkanConvertAccessFlags(barrier.dstAccess);
            imageMemoryBarrier.srcStageMask = VulkanConvertPipelineStages(barrier.srcStages);
            imageMemoryBarrier.dstStageMask = VulkanConvertPipelineStages(barrier.dstStages);
            imageMemoryBarrier.oldLayout = VulkanConvertImageResourceState(barrier.oldState);
            imageMemoryBarrier.newLayout = VulkanConvertImageResourceState(barrier.newState);
            imageMemoryBarrier.srcQueueFamilyIndex =
                barrier.srcQueue ? barrier.srcQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex =
                barrier.dstQueue ? barrier.dstQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = barrier.image->GetHandle();
            imageMemoryBarrier.subresourceRange.aspectMask = VulkanConvertImagePlanes(barrier.subresourceRange.planes);
            imageMemoryBarrier.subresourceRange.baseMipLevel = barrier.subresourceRange.baseMipLevel;
            imageMemoryBarrier.subresourceRange.levelCount = barrier.subresourceRange.levelCount;
            imageMemoryBarrier.subresourceRange.baseArrayLayer = barrier.subresourceRange.baseArrayLayer;
            imageMemoryBarrier.subresourceRange.layerCount = barrier.subresourceRange.layerCount;
            imageMemoryBarriers.push_back(imageMemoryBarrier);
        }
        dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();

        m_CommandBuffer.pipelineBarrier2(dependencyInfo);
        return true;
    }

    void RHI_VK_FUNC_IMPL(RHICommandBuffer, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* commandPoolOwner = m_CommandPoolOwner;
        ReleaseWithoutUnregister();

        if (commandPoolOwner && !m_IsDetached)
        {
            commandPoolOwner->UnregisterCommandBuffer(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHICommandBuffer, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* commandPoolOwner = m_CommandPoolOwner;
        ReleaseImmediateWithoutUnregister();

        if (commandPoolOwner && !m_IsDetached)
        {
            commandPoolOwner->UnregisterCommandBuffer(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHICommandBuffer, ReleaseWithoutUnregister)()
    {
        if (!m_IsValid)
        {
            return;
        }

        const auto device = m_Device;
        const auto commandPool = m_CommandPool;
        const auto commandBuffer = m_CommandBuffer;
        if (m_CommandPoolOwner)
        {
            m_CommandPoolOwner->EnqueueDeletion([device, commandPool, commandBuffer]() {
                if (device && commandPool && commandBuffer)
                {
                    device.freeCommandBuffers(commandPool, commandBuffer);
                }
            });
        }
        else if (device && commandPool && commandBuffer)
        {
            device.freeCommandBuffers(commandPool, commandBuffer);
        }

        m_CommandBuffer = VK_NULL_HANDLE;
        m_IsRecording = false;
        m_IsRendering = false;
        m_IsValid = false;
        m_CommandPoolOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_CommandPool = VK_NULL_HANDLE;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHICommandBuffer, ReleaseImmediateWithoutUnregister)()
    {
        if (!m_IsValid)
        {
            return;
        }

        if (m_Device && m_CommandPool && m_CommandBuffer)
        {
            m_Device.freeCommandBuffers(m_CommandPool, m_CommandBuffer);
        }

        m_CommandBuffer = VK_NULL_HANDLE;
        m_IsRecording = false;
        m_IsRendering = false;
        m_IsValid = false;
        m_CommandPoolOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_CommandPool = VK_NULL_HANDLE;
        m_IsDetached = false;
    }
} // namespace Hazel