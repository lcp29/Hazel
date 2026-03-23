//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"
#include "VulkanImageView.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHICommandBuffer)
    {
    public:
        bool IsValid() const
        {
            return m_IsValid;
        }

        bool IsRecording() const
        {
            return m_IsRecording;
        }

        bool Begin(bool oneTimeSubmit);
        bool End();
        bool Reset();
        bool BeginRendering(const RHIRenderingInfo& info);
        bool EndRendering();
        bool BindGraphicsPipeline(RHIGraphicsPipeline* pipeline);
        bool BindComputePipeline(RHIComputePipeline* pipeline);
        bool BindVertexBuffer(uint32_t binding, RHIBuffer* buffer, uint64_t offset = 0);
        bool BindIndexBuffer(RHIBuffer* buffer, RHIIndexType indexType, uint64_t offset = 0);
        bool BindGraphicsResourceGroup(RHIGraphicsPipeline* pipeline,
                                       uint32_t set,
                                       RHIResourceGroup* resourceGroup,
                                       const std::vector<uint32_t>* bufferOffsets = {});
        bool BindGraphicsResourceGroup(RHIResourceSignature* signature,
                                       uint32_t set,
                                       RHIResourceGroup* resourceGroup,
                                       const std::vector<uint32_t>* bufferOffsets = {});
        bool BindComputeResourceGroup(RHIComputePipeline* pipeline, uint32_t set, RHIResourceGroup* resourceGroup);
        bool BindComputeResourceGroup(RHIResourceSignature* signature, uint32_t set, RHIResourceGroup* resourceGroup);
        bool BlitImage(RHIImage* srcImage,
                       RHIImageResourceState srcState,
                       RHIImage* dstImage,
                       RHIImageResourceState dstState,
                       const RHIImageBlitDesc& desc);
        bool CopyBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, const RHIBufferCopyDesc& desc);
        bool CopyBufferToImage(RHIBuffer* srcBuffer,
                               uint64_t bufferOffset,
                               RHIExtent2D bufferMemoryExtent,
                               RHIImage* dstImage,
                               RHIOffset3D dstOffset,
                               RHIExtent3D dstExtent,
                               RHIImageSubresourceLayers dstSubresource);
        bool CopyImageToBuffer(RHIImage* srcImage,
                               RHIBuffer* dstBuffer,
                               uint64_t bufferOffset,
                               RHIExtent2D bufferMemoryExtent,
                               RHIOffset3D srcOffset,
                               RHIExtent3D srcExtent,
                               RHIImageSubresourceLayers srcSubresource);
        bool ClearAttachments(const RHIClearAttachmentsDesc& desc);
        bool ClearColorImage(RHIImage* image,
                             RHIImageResourceState state,
                             const RHIClearColorValue& value,
                             const RHIImageSubresourceRange& subresourceRange);
        bool ClearDepthStencilImage(RHIImage* image,
                                    RHIImageResourceState state,
                                    const RHIClearDepthStencilValue& value,
                                    const RHIImageSubresourceRange& subresourceRange);
        bool Draw(uint32_t vertexCount,
                  uint32_t instanceCount = 1,
                  uint32_t firstVertex = 0,
                  uint32_t firstInstance = 0);
        bool DrawIndexed(uint32_t indexCount,
                         uint32_t instanceCount = 1,
                         uint32_t firstIndex = 0,
                         int32_t vertexOffset = 0,
                         uint32_t firstInstance = 0);
        bool DrawIndirect(RHIBuffer* buffer, uint64_t offset = 0, uint32_t drawCount = 1, uint32_t stride = 0);
        bool DrawIndexedIndirect(RHIBuffer* buffer, uint64_t offset = 0, uint32_t drawCount = 1, uint32_t stride = 0);
        bool Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
        bool DispatchIndirect(RHIBuffer* buffer, uint64_t offset = 0);
        bool SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
        bool SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);
        bool SetBlendConstants(float red, float green, float blue, float alpha);
        bool SetStencilReference(uint32_t reference);
        bool PushConstants(RHIResourceSignature* signature,
                           const RHIShaderStages& stages,
                           uint32_t offset,
                           uint32_t size,
                           const void* data);
        bool PipelineBarriers(const RHIPipelineBarrierDesc& desc);

        void Release();
        void ReleaseImmediate();
        ~RHICommandBufferImpl();

        const RHICommandBufferDesc& GetDesc() const
        {
            return m_Desc;
        }

        vk::CommandBuffer GetHandle() const
        {
            return m_CommandBuffer;
        }

        bool IsDetached() const
        {
            return m_IsDetached;
        }

    private:
        friend class RHICommandPoolImpl<RHIBackend::Vulkan>;

        RHICommandBufferImpl(RHICommandPool* commandPoolOwner,
                             vk::Device device,
                             vk::CommandPool commandPool,
                             const RHICommandBufferDesc& desc);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        bool m_IsRecording = false;
        bool m_IsRendering = false;
        RHICommandBufferDesc m_Desc;
        RHICommandPool* m_CommandPoolOwner = nullptr;
        vk::Device m_Device;
        vk::CommandPool m_CommandPool = VK_NULL_HANDLE;
        vk::CommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        bool m_IsDetached = false;
    };
} // namespace Hazel