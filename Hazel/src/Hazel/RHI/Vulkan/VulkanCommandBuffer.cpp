//
// Created by helmholtz on 2026/3/15.
//

#define VULKAN_HPP_NO_EXCEPTIONS

#include "VulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanCommon.h"
#include "VulkanQueue.h"
#include "VulkanCommandPool.h"
#include "VulkanComputePipeline.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanImageView.h"
#include "VulkanResourceGroup.h"
#include "VulkanResourceSignature.h"
#include "VulkanPipelineCommon.h"
#include "VulkanImage.h"

#include <array>

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

        bool VulkanIsDepthFormat(RHIFormat format)
        {
            return format == RHIFormat::D32SFloat || format == RHIFormat::D32SFloatS8Uint;
        }

        bool VulkanIsStencilFormat(RHIFormat format)
        {
            return format == RHIFormat::S8Uint || format == RHIFormat::D32SFloatS8Uint;
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHICommandBuffer, RHICommandBufferImpl)(RHICommandPool *commandPoolOwner,
                                                             vk::Device device,
                                                             vk::CommandPool commandPool,
                                                             const RHICommandBufferDesc &desc)
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
        if (!m_IsValid || m_IsRecording)
        {
            return false;
        }

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
        if (!m_IsValid || !m_IsRecording || m_IsRendering)
        {
            return false;
        }

        if (m_CommandBuffer.end() != vk::Result::eSuccess)
        {
            return false;
        }

        m_IsRecording = false;
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, Reset)()
    {
        if (!m_IsValid || m_IsRecording || m_IsRendering)
        {
            return false;
        }

        if (m_CommandBuffer.reset() != vk::Result::eSuccess)
        {
            return false;
        }

        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BeginRendering)(const RHIRenderingInfo &info)
    {
        if (!m_IsValid || !m_IsRecording || m_IsRendering)
        {
            return false;
        }
        if (info.colorAttachments.empty() && !info.depthStencilAttachment.has_value())
        {
            return false;
        }
        if (info.renderAreaWidth == 0 || info.renderAreaHeight == 0 || info.layerCount == 0)
        {
            return false;
        }

        std::vector<vk::RenderingAttachmentInfo> colorAttachments;
        colorAttachments.reserve(info.colorAttachments.size());
        for (const auto &attachment: info.colorAttachments)
        {
            auto *imageView = attachment.imageView;
            if (!imageView || !imageView->IsValid())
            {
                return false;
            }

            vk::RenderingAttachmentInfo attachmentInfo;
            attachmentInfo.imageView = imageView->GetHandle();
            attachmentInfo.imageLayout = VulkanConvertResourceState(attachment.state);
            attachmentInfo.loadOp = VulkanConvertRenderingLoadOp(attachment.loadOp);
            attachmentInfo.storeOp = VulkanConvertRenderingStoreOp(attachment.storeOp);
            attachmentInfo.clearValue.color = vk::ClearColorValue(std::array<float, 4>{
                attachment.clearColor[0],
                attachment.clearColor[1],
                attachment.clearColor[2],
                attachment.clearColor[3]
            });
            colorAttachments.push_back(attachmentInfo);
        }

        std::optional<vk::RenderingAttachmentInfo> depthAttachment;
        std::optional<vk::RenderingAttachmentInfo> stencilAttachment;
        if (info.depthStencilAttachment.has_value())
        {
            const auto &attachment = info.depthStencilAttachment.value();
            auto *imageView = attachment.imageView;
            if (!imageView || !imageView->IsValid())
            {
                return false;
            }

            const auto format = attachment.imageView->GetFormat();
            const bool hasDepthAspect = VulkanIsDepthFormat(format);
            const bool hasStencilAspect = VulkanIsStencilFormat(format);
            if (!hasDepthAspect && !hasStencilAspect)
            {
                return false;
            }
            if (hasDepthAspect)
            {
                depthAttachment.emplace();
                depthAttachment->imageView = imageView->GetHandle();
                depthAttachment->imageLayout = VulkanConvertResourceState(attachment.state);
                depthAttachment->loadOp = VulkanConvertRenderingLoadOp(attachment.depthLoadOp);
                depthAttachment->storeOp = VulkanConvertRenderingStoreOp(attachment.depthStoreOp);
                depthAttachment->clearValue.depthStencil = vk::ClearDepthStencilValue(
                    attachment.clearDepth,
                    attachment.clearStencil);
            }

            if (hasStencilAspect)
            {
                stencilAttachment.emplace();
                stencilAttachment->imageView = imageView->GetHandle();
                stencilAttachment->imageLayout = VulkanConvertResourceState(attachment.state);
                stencilAttachment->loadOp = VulkanConvertRenderingLoadOp(attachment.stencilLoadOp);
                stencilAttachment->storeOp = VulkanConvertRenderingStoreOp(attachment.stencilStoreOp);
                stencilAttachment->clearValue.depthStencil = vk::ClearDepthStencilValue(
                    attachment.clearDepth,
                    attachment.clearStencil);
            }
        }

        vk::RenderingInfo renderingInfo;
        renderingInfo.renderArea = vk::Rect2D(
            vk::Offset2D(info.renderAreaX, info.renderAreaY),
            vk::Extent2D(info.renderAreaWidth, info.renderAreaHeight));
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
        if (!m_IsValid || !m_IsRecording || !m_IsRendering)
        {
            return false;
        }

        m_CommandBuffer.endRendering();
        m_IsRendering = false;
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindGraphicsPipeline)(RHIGraphicsPipeline *pipeline)
    {
        auto *vkPipeline = pipeline;
        if (!m_IsValid || !m_IsRecording || !vkPipeline || !vkPipeline->IsValid())
        {
            return false;
        }

        m_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipeline->GetHandle());
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindComputePipeline)(RHIComputePipeline *pipeline)
    {
        auto *vkPipeline = pipeline;
        if (!m_IsValid || !m_IsRecording || !vkPipeline || !vkPipeline->IsValid())
        {
            return false;
        }

        m_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, vkPipeline->GetHandle());
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindVertexBuffer)(uint32_t binding, RHIBuffer *buffer, uint64_t offset)
    {
        auto *vkBuffer = buffer;
        if (!m_IsValid || !m_IsRecording || !vkBuffer || !vkBuffer->IsValid())
        {
            return false;
        }
        if (!(buffer->GetUsages() & RHIBufferUsageFlagBits::VertexBuffer) || offset >= buffer->GetSize())
        {
            return false;
        }

        const vk::Buffer handle = vkBuffer->GetHandle();
        const vk::DeviceSize deviceOffset = offset;
        m_CommandBuffer.bindVertexBuffers(binding, 1, &handle, &deviceOffset);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindIndexBuffer)(RHIBuffer *buffer, RHIIndexType indexType, uint64_t offset)
    {
        auto *vkBuffer = buffer;
        if (!m_IsValid || !m_IsRecording || !vkBuffer || !vkBuffer->IsValid())
        {
            return false;
        }
        if (!(buffer->GetUsages() & RHIBufferUsageFlagBits::IndexBuffer) || offset >= buffer->GetSize())
        {
            return false;
        }

        m_CommandBuffer.bindIndexBuffer(vkBuffer->GetHandle(), offset, VulkanConvertIndexType(indexType));
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindGraphicsResourceGroup)(RHIGraphicsPipeline *pipeline,
                                                                       uint32_t set,
                                                                       RHIResourceGroup *resourceGroup)
    {
        auto *vkPipeline = pipeline;
        if (!m_IsValid || !m_IsRecording || !vkPipeline || !vkPipeline->IsValid())
        {
            return false;
        }

        return BindGraphicsResourceGroup(vkPipeline->GetDesc().resourceSignature, set, resourceGroup);
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindGraphicsResourceGroup)(RHIResourceSignature *signature,
                                                                       uint32_t set,
                                                                       RHIResourceGroup *resourceGroup)
    {
        auto *vkSignature = signature;
        auto *vkGroup = resourceGroup;
        if (!m_IsValid || !m_IsRecording || !vkSignature || !vkGroup || !vkSignature->IsValid() || !vkGroup->IsValid())
        {
            return false;
        }

        const auto &layouts = signature->GetDesc().resourceLayouts;
        if (set >= layouts.size() || layouts[set] != vkGroup->GetLayout())
        {
            return false;
        }

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

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindComputeResourceGroup)(RHIComputePipeline *pipeline,
                                                                      uint32_t set,
                                                                      RHIResourceGroup *resourceGroup)
    {
        auto *vkPipeline = pipeline;
        if (!m_IsValid || !m_IsRecording || !vkPipeline || !vkPipeline->IsValid())
        {
            return false;
        }

        return BindComputeResourceGroup(vkPipeline->GetDesc().resourceSignature, set, resourceGroup);
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, BindComputeResourceGroup)(RHIResourceSignature *signature,
                                                                      uint32_t set,
                                                                      RHIResourceGroup *resourceGroup)
    {
        auto *vkSignature = signature;
        auto *vkGroup = resourceGroup;
        if (!m_IsValid || !m_IsRecording || !vkSignature || !vkGroup || !vkSignature->IsValid() || !vkGroup->IsValid())
        {
            return false;
        }

        const auto &layouts = signature->GetDesc().resourceLayouts;
        if (set >= layouts.size() || layouts[set] != vkGroup->GetLayout())
        {
            return false;
        }

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

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, Draw)(uint32_t vertexCount,
                                                  uint32_t instanceCount,
                                                  uint32_t firstVertex,
                                                  uint32_t firstInstance)
    {
        if (!m_IsValid || !m_IsRecording || !m_IsRendering || vertexCount == 0 || instanceCount == 0)
        {
            return false;
        }

        m_CommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, DrawIndexed)(uint32_t indexCount,
                                                         uint32_t instanceCount,
                                                         uint32_t firstIndex,
                                                         int32_t vertexOffset,
                                                         uint32_t firstInstance)
    {
        if (!m_IsValid || !m_IsRecording || !m_IsRendering || indexCount == 0 || instanceCount == 0)
        {
            return false;
        }

        m_CommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, DrawIndirect)(RHIBuffer *buffer,
                                                          uint64_t offset,
                                                          uint32_t drawCount,
                                                          uint32_t stride)
    {
        if (!m_IsValid || !m_IsRecording || !m_IsRendering || !buffer || !buffer->IsValid() || drawCount == 0)
        {
            return false;
        }
        if (!(buffer->GetUsages() & RHIBufferUsageFlagBits::IndirectBuffer) || offset >= buffer->GetSize())
        {
            return false;
        }

        const uint32_t resolvedStride = stride == 0 ? s_DrawIndirectCommandSize : stride;
        if (resolvedStride < s_DrawIndirectCommandSize)
        {
            return false;
        }

        const uint64_t requiredSize = offset + static_cast<uint64_t>(resolvedStride) * drawCount;
        if (requiredSize > buffer->GetSize())
        {
            return false;
        }

        m_CommandBuffer.drawIndirect(buffer->GetHandle(), offset, drawCount, resolvedStride);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, DrawIndexedIndirect)(RHIBuffer *buffer,
                                                                 uint64_t offset,
                                                                 uint32_t drawCount,
                                                                 uint32_t stride)
    {
        if (!m_IsValid || !m_IsRecording || !m_IsRendering || !buffer || !buffer->IsValid() || drawCount == 0)
        {
            return false;
        }
        if (!(buffer->GetUsages() & RHIBufferUsageFlagBits::IndirectBuffer) || offset >= buffer->GetSize())
        {
            return false;
        }

        const uint32_t resolvedStride = stride == 0 ? s_DrawIndexedIndirectCommandSize : stride;
        if (resolvedStride < s_DrawIndexedIndirectCommandSize)
        {
            return false;
        }

        const uint64_t requiredSize = offset + static_cast<uint64_t>(resolvedStride) * drawCount;
        if (requiredSize > buffer->GetSize())
        {
            return false;
        }

        m_CommandBuffer.drawIndexedIndirect(buffer->GetHandle(), offset, drawCount, resolvedStride);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, SetViewport)(float x,
                                                         float y,
                                                         float width,
                                                         float height,
                                                         float minDepth,
                                                         float maxDepth)
    {
        if (!m_IsValid || !m_IsRecording)
        {
            return false;
        }

        const vk::Viewport viewport(x, y, width, height, minDepth, maxDepth);
        m_CommandBuffer.setViewport(0, viewport);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, SetScissor)(int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        if (!m_IsValid || !m_IsRecording)
        {
            return false;
        }

        const vk::Rect2D scissor({x, y}, {width, height});
        m_CommandBuffer.setScissor(0, scissor);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, SetBlendConstants)(float red, float green, float blue, float alpha)
    {
        if (!m_IsValid || !m_IsRecording)
        {
            return false;
        }

        const float blendConstants[] = {red, green, blue, alpha};
        m_CommandBuffer.setBlendConstants(blendConstants);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, SetStencilReference)(uint32_t reference)
    {
        if (!m_IsValid || !m_IsRecording)
        {
            return false;
        }

        m_CommandBuffer.setStencilReference(vk::StencilFaceFlagBits::eFront | vk::StencilFaceFlagBits::eBack,
                                            reference);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, PushConstants)(RHIResourceSignature *signature,
                                                           RHIShaderStages stages,
                                                           uint32_t offset,
                                                           uint32_t size,
                                                           const void *data)
    {
        auto *vkSignature = signature;
        if (!m_IsValid || !m_IsRecording || !vkSignature || !vkSignature->IsValid() || !data)
        {
            return false;
        }

        bool hasMatchingRange = false;
        for (const auto &pushConstantRange: signature->GetDesc().pushConstantRanges)
        {
            const uint32_t rangeEnd = pushConstantRange.offset + pushConstantRange.size;
            const uint32_t pushEnd = offset + size;
            const bool stageMatch = (pushConstantRange.stages.value & stages.value) == stages.value;
            if (stageMatch && offset >= pushConstantRange.offset && pushEnd <= rangeEnd)
            {
                hasMatchingRange = true;
                break;
            }
        }

        if (!hasMatchingRange)
        {
            return false;
        }

        m_CommandBuffer.pushConstants(
            vkSignature->GetPipelineLayout(),
            VulkanConvertShaderStages(stages),
            offset,
            size,
            data);
        return true;
    }

    bool RHI_VK_FUNC_IMPL(RHICommandBuffer, PipelineBarriers)(const RHIPipelineBarrierDesc &desc)
    {
        if (!m_IsRecording)
        {
            return false;
        }
        vk::DependencyInfo dependencyInfo;

        dependencyInfo.memoryBarrierCount = desc.memoryBarriers.size();
        std::vector<vk::MemoryBarrier2> memoryBarriers;
        memoryBarriers.reserve(dependencyInfo.memoryBarrierCount);
        for (const auto &barrier: desc.memoryBarriers)
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
        for (const auto &barrier: desc.bufferBarriers)
        {
            if (!barrier.buffer)
            {
                return false;
            }
            vk::BufferMemoryBarrier2 bufferMemoryBarrier;
            bufferMemoryBarrier.srcAccessMask = VulkanConvertAccessFlags(barrier.srcAccess);
            bufferMemoryBarrier.dstAccessMask = VulkanConvertAccessFlags(barrier.dstAccess);
            bufferMemoryBarrier.srcStageMask = VulkanConvertPipelineStages(barrier.srcStages);
            bufferMemoryBarrier.dstStageMask = VulkanConvertPipelineStages(barrier.dstStages);
            bufferMemoryBarrier.srcQueueFamilyIndex = barrier.srcQueue ? barrier.srcQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.dstQueueFamilyIndex = barrier.dstQueue ? barrier.dstQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.buffer = barrier.buffer->GetHandle();
            bufferMemoryBarrier.offset = barrier.offset;
            bufferMemoryBarrier.size = barrier.size;
            bufferMemoryBarriers.push_back(bufferMemoryBarrier);
        }
        dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();

        dependencyInfo.imageMemoryBarrierCount = desc.imageBarriers.size();
        std::vector<vk::ImageMemoryBarrier2> imageMemoryBarriers;
        imageMemoryBarriers.reserve(dependencyInfo.imageMemoryBarrierCount);
        for (const auto &barrier: desc.imageBarriers)
        {
            if (!barrier.image)
            {
                return false;
            }
            vk::ImageMemoryBarrier2 imageMemoryBarrier;
            imageMemoryBarrier.srcAccessMask = VulkanConvertAccessFlags(barrier.srcAccess);
            imageMemoryBarrier.dstAccessMask = VulkanConvertAccessFlags(barrier.dstAccess);
            imageMemoryBarrier.srcStageMask = VulkanConvertPipelineStages(barrier.srcStages);
            imageMemoryBarrier.dstStageMask = VulkanConvertPipelineStages(barrier.dstStages);
            imageMemoryBarrier.oldLayout = VulkanConvertResourceState(barrier.oldState);
            imageMemoryBarrier.newLayout = VulkanConvertResourceState(barrier.newState);
            imageMemoryBarrier.srcQueueFamilyIndex = barrier.srcQueue ? barrier.srcQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = barrier.dstQueue ? barrier.dstQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED;
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

        auto *commandPoolOwner = m_CommandPoolOwner;
        ReleaseWithoutUnregister();

        if (commandPoolOwner)
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

        auto *commandPoolOwner = m_CommandPoolOwner;
        ReleaseImmediateWithoutUnregister();

        if (commandPoolOwner)
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
            m_CommandPoolOwner->EnqueueDeletion([device, commandPool, commandBuffer]()
            {
                if (device && commandPool && commandBuffer)
                {
                    device.freeCommandBuffers(commandPool, commandBuffer);
                }
            });
        } else if (device && commandPool && commandBuffer)
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
    }
} // Hazel
