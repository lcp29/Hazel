//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanAdapter.h"
#include "VulkanBase.h"

#include <array>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace Hazel
{
    class VulkanMemoryAllocator;

    RHI_VK_CLASS_IMPL(RHIDevice)
    {
    public:
        bool IsValid() const
        {
            return m_IsValid;
        }

        bool WaitIdle();
        bool WaitSyncPoint(RHISyncPoint* syncPoint, uint64_t timeout = UINT64_MAX);
        void FlushDeferredDeletions();

        RHIQueue* GetQueue(RHIQueueType type) const;

        RHIQueue* GetUniformQueue() const
        {
            return m_IsUniformQueue ? GetQueue(RHIQueueType::Graphics) : nullptr;
        }

        RHIBuffer* CreateBuffer(const RHIBufferDesc& desc, bool isDetached = false);
        RHIBufferView* CreateBufferView(RHIBuffer* buffer, const RHIBufferViewDesc& desc, bool isDetached = false);
        RHICommandPool* CreateCommandPool(const RHICommandPoolDesc& desc, bool isDetached = false);
        RHICommandPool* CreateCommandPoolUniformQueue(const RHICommandPoolDesc& desc, bool isDetached = false);
        RHIComputePipeline* CreateComputePipeline(const RHIComputePipelineDesc& desc, bool isDetached = false);
        RHIGraphicsPipeline* CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc, bool isDetached = false);
        RHIImage* CreateImage(const RHIImageDesc& desc, bool isDetached = false);
        RHIResourceHeap* CreateResourceHeap(const RHIResourceHeapDesc& desc, bool isDetached = false);
        RHIResourceLayout* CreateResourceLayout(const RHIResourceLayoutDesc& desc, bool isDetached = false);
        RHIResourceSignature* CreateResourceSignature(const RHIResourceSignatureDesc& desc, bool isDetached = false);
        RHISampler* CreateSampler(const RHISamplerDesc& desc, bool isDetached = false);
        RHIShader* CreateShader(const RHIShaderDesc& desc, bool isDetached = false);
        RHISwapchain* CreateSwapchain(const RHISwapchainDesc& desc, bool isDetached = false);
        RHIImageView* CreateImageView(RHIImage* image, const RHIImageViewDesc& desc, bool isDetached = false);
        void Release();
        ~RHIDeviceImpl();

        const RHIAdapter& GetAdapter() const
        {
            return m_Adapter;
        }

        const RHIDeviceCapabilities& GetCapabilities() const
        {
            return m_Capabilities;
        }

        vk::Device GetHandle() const
        {
            return m_Device;
        }

        vk::PhysicalDevice GetPhysicalDevice() const
        {
            return m_PhysicalDevice;
        }

        VulkanMemoryAllocator* GetAllocator() const
        {
            return m_Allocator.get();
        }

        bool IsUniformQueue() const
        {
            return m_IsUniformQueue;
        }

        void EnqueueDeletion(DeletionQueue::Operation operation)
        {
            m_DeletionQueue.Enqueue(std::move(operation));
        }

        DeletionQueue::OperationSet ExtractDeletionQueue()
        {
            return m_DeletionQueue.ExtractAll();
        }

        RHIDeviceImpl(RHIDevice&& device) noexcept;

    private:
        friend class RHIInstanceImpl<RHIBackend::Vulkan>;
        friend class RHISwapchainImpl<RHIBackend::Vulkan>;
        friend class RHICommandPoolImpl<RHIBackend::Vulkan>;
        friend class RHIBufferImpl<RHIBackend::Vulkan>;
        friend class RHIImageImpl<RHIBackend::Vulkan>;
        friend class RHIComputePipelineImpl<RHIBackend::Vulkan>;
        friend class RHIGraphicsPipelineImpl<RHIBackend::Vulkan>;
        friend class RHIResourceHeapImpl<RHIBackend::Vulkan>;
        friend class RHIResourceLayoutImpl<RHIBackend::Vulkan>;
        friend class RHIResourceSignatureImpl<RHIBackend::Vulkan>;
        friend class RHISamplerImpl<RHIBackend::Vulkan>;
        friend class RHIShaderImpl<RHIBackend::Vulkan>;

        RHIDeviceImpl(RHIInstance* instanceOwner,
                      vk::Instance instance,
                      const RHIAdapter& adapter,
                      const RHIDeviceCapabilities& caps,
                      const RHISurface* surface = nullptr);
        RHIDeviceImpl(const RHIDeviceImpl&) = delete;
        RHIDeviceImpl& operator=(const RHIDeviceImpl&) = delete;

        static size_t GetQueueSlot(RHIQueueType type);

        void ReleaseWithoutUnregister();
        void RegisterSwapchain(std::unique_ptr<RHISwapchain> swapchain);
        void UnregisterSwapchain(RHISwapchain* swapchain);
        void RegisterCommandPool(std::unique_ptr<RHICommandPool> commandPool);
        void UnregisterCommandPool(RHICommandPool* commandPool);
        void RegisterBuffer(std::unique_ptr<RHIBuffer> buffer);
        void UnregisterBuffer(RHIBuffer* buffer);
        void RegisterImage(std::unique_ptr<RHIImage> image);
        void UnregisterImage(RHIImage* image);
        void RegisterComputePipeline(std::unique_ptr<RHIComputePipeline> pipeline);
        void UnregisterComputePipeline(RHIComputePipeline* pipeline);
        void RegisterGraphicsPipeline(std::unique_ptr<RHIGraphicsPipeline> pipeline);
        void UnregisterGraphicsPipeline(RHIGraphicsPipeline* pipeline);
        void RegisterResourceHeap(std::unique_ptr<RHIResourceHeap> heap);
        void UnregisterResourceHeap(RHIResourceHeap* heap);
        void RegisterResourceLayout(std::unique_ptr<RHIResourceLayout> layout);
        void UnregisterResourceLayout(RHIResourceLayout* layout);
        void RegisterResourceSignature(std::unique_ptr<RHIResourceSignature> signature);
        void UnregisterResourceSignature(RHIResourceSignature* signature);
        void RegisterSampler(std::unique_ptr<RHISampler> sampler);
        void UnregisterSampler(RHISampler* sampler);
        void RegisterShader(std::unique_ptr<RHIShader> shader);
        void UnregisterShader(RHIShader* shader);

        bool m_IsValid = false;
        bool m_IsUniformQueue = false;
        RHIInstance* m_InstanceOwner = nullptr;
        RHIAdapter m_Adapter;
        RHIDeviceCapabilities m_Capabilities;
        DeletionQueue m_DeletionQueue;
        vk::Instance m_Instance;
        vk::PhysicalDevice m_PhysicalDevice;
        vk::Device m_Device;
        std::array<RHIQueue*, 4> m_QueueLookup{};
        RHIOwnerSet<RHIQueue> m_Queues;
        std::unique_ptr<VulkanMemoryAllocator> m_Allocator;
        RHIOwnerSet<RHICommandPool> m_CommandPools;
        RHIOwnerSet<RHIBuffer> m_Buffers;
        RHIOwnerSet<RHIComputePipeline> m_ComputePipelines;
        RHIOwnerSet<RHIGraphicsPipeline> m_GraphicsPipelines;
        RHIOwnerSet<RHIImage> m_Images;
        RHIOwnerSet<RHIResourceHeap> m_ResourceHeaps;
        RHIOwnerSet<RHIResourceLayout> m_ResourceLayouts;
        RHIOwnerSet<RHIResourceSignature> m_ResourceSignatures;
        RHIOwnerSet<RHISampler> m_Samplers;
        RHIOwnerSet<RHIShader> m_Shaders;
        RHIOwnerSet<RHISwapchain> m_Swapchains;
    };
} // namespace Hazel