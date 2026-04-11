//
// Created by helmholtz on 2026/3/14.
//

#define VULKAN_HPP_NO_EXCEPTIONS

#include "VulkanDevice.h"

#include "../RHIBase.h"
#include "../RHIDevice.h"
#include "VulkanBuffer.h"
#include "VulkanBufferView.h"
#include "VulkanCommandPool.h"
#include "VulkanComputePipeline.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanInstance.h"
#include "VulkanMemoryAllocator.h"
#include "VulkanQueue.h"
#include "VulkanResourceHeap.h"
#include "VulkanResourceLayout.h"
#include "VulkanResourceSignature.h"
#include "VulkanSampler.h"
#include "VulkanShader.h"
#include "VulkanSurface.h"
#include "VulkanSwapchain.h"

#include <set>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Hazel
{
    namespace
    {
        constexpr float s_QueuePriority = 1.0f;

        bool QueueFlagsMatch(vk::QueueFlags flags, vk::QueueFlagBits bit)
        {
            return (flags & bit) == bit;
        }

        std::optional<uint32_t> FindQueueFamilyIndex(const std::vector<vk::QueueFamilyProperties>& queueFamilies,
                                                     RHIQueueType type)
        {
            const auto matches = [type](const vk::QueueFamilyProperties& queueFamily) {
                switch (type)
                {
                    case RHIQueueType::Graphics:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eGraphics);
                    case RHIQueueType::Compute:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eCompute);
                    case RHIQueueType::Transfer:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eTransfer);
                    case RHIQueueType::Present:
                        return false;
                }

                return false;
            };

            const auto preferred = [type](const vk::QueueFamilyProperties& queueFamily) {
                switch (type)
                {
                    case RHIQueueType::Graphics:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eGraphics);
                    case RHIQueueType::Compute:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eCompute);
                    case RHIQueueType::Transfer:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eTransfer);
                    case RHIQueueType::Present:
                        return false;
                }

                return false;
            };

            for (uint32_t i = 0; i < queueFamilies.size(); i++)
            {
                if (queueFamilies[i].queueCount > 0 && preferred(queueFamilies[i])) { return i; }
            }

            for (uint32_t i = 0; i < queueFamilies.size(); i++)
            {
                if (queueFamilies[i].queueCount > 0 && matches(queueFamilies[i])) { return i; }
            }

            return std::nullopt;
        }

        std::optional<uint32_t> FindPresentQueueFamilyIndex(vk::PhysicalDevice physicalDevice,
                                                            const std::vector<vk::QueueFamilyProperties>& queueFamilies,
                                                            vk::SurfaceKHR surface)
        {
            for (uint32_t i = 0; i < queueFamilies.size(); i++)
            {
                if (queueFamilies[i].queueCount == 0) { continue; }
                if (physicalDevice.getSurfaceSupportKHR(i, surface).value) { return i; }
            }

            return std::nullopt;
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHIDevice, RHIDeviceImpl)(RHIInstance* instanceOwner,
                                               vk::Instance instance,
                                               const RHIAdapter& adapter,
                                               const RHIDeviceCapabilities& caps,
                                               const RHISurface* surface)
        : m_InstanceOwner(instanceOwner)
          , m_Adapter(adapter)
          , m_Capabilities(caps)
          , m_Instance(instance)
    {
        if (!adapter.CanCreateDevice(caps))
        {
            return;
        }

        m_PhysicalDevice = adapter.GetHandle();

        const auto queueFamilies = m_PhysicalDevice.getQueueFamilyProperties();
        std::vector<RHIQueueType> requestedQueues;
        if (caps.queueTypes & RHIQueueTypeFlagBits::Graphics)
        {
            requestedQueues.push_back(RHIQueueType::Graphics);
        }
        if (caps.queueTypes & RHIQueueTypeFlagBits::Compute)
        {
            requestedQueues.push_back(RHIQueueType::Compute);
        }
        if (caps.queueTypes & RHIQueueTypeFlagBits::Transfer)
        {
            requestedQueues.push_back(RHIQueueType::Transfer);
        }
        if (caps.queueTypes & RHIQueueTypeFlagBits::Present)
        {
            requestedQueues.push_back(RHIQueueType::Present);
        }
        if (requestedQueues.empty())
        {
            const auto availableQueues = adapter.GetCapabilities().queueTypes;
            if (availableQueues & RHIQueueTypeFlagBits::Graphics)
            {
                requestedQueues.push_back(RHIQueueType::Graphics);
            }
            if (availableQueues & RHIQueueTypeFlagBits::Compute)
            {
                requestedQueues.push_back(RHIQueueType::Compute);
            }
            if (availableQueues & RHIQueueTypeFlagBits::Transfer)
            {
                requestedQueues.push_back(RHIQueueType::Transfer);
            }
        }

        const bool requiresPresentQueue = caps.queueTypes & RHIQueueTypeFlagBits::Present;
        if (requiresPresentQueue && (!surface || !surface->IsValid()))
        {
            return;
        }

        std::array<std::optional<uint32_t>, 4> queueFamilyIndices{};
        for (const auto queueType : requestedQueues)
        {
            std::optional<uint32_t> queueFamilyIndex;
            if (queueType == RHIQueueType::Present)
            {
                queueFamilyIndex = FindPresentQueueFamilyIndex(m_PhysicalDevice, queueFamilies, surface->GetHandle());
            }
            else
            {
                queueFamilyIndex = FindQueueFamilyIndex(queueFamilies, queueType);
            }

            if (!queueFamilyIndex.has_value())
            {
                return;
            }

            queueFamilyIndices[GetQueueSlot(queueType)] = queueFamilyIndex.value();
        }

        std::set<uint32_t> uniqueQueueFamilies;
        for (const auto& queueFamilyIndex : queueFamilyIndices)
        {
            if (queueFamilyIndex.has_value())
            {
                uniqueQueueFamilies.insert(queueFamilyIndex.value());
            }
        }

        m_IsUniformQueue = uniqueQueueFamilies.size() == 1;

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(uniqueQueueFamilies.size());
        for (const auto queueFamilyIndex : uniqueQueueFamilies)
        {
            queueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), queueFamilyIndex, 1, &s_QueuePriority);
        }

        vk::PhysicalDeviceFeatures2 enabledFeatures;
        vk::PhysicalDeviceVulkan11Features vulkan11Features;
        vk::PhysicalDeviceVulkan12Features vulkan12Features;
        vk::PhysicalDeviceVulkan13Features vulkan13Features;
        enabledFeatures.pNext = &vulkan11Features;
        vulkan11Features.pNext = &vulkan12Features;
        vulkan12Features.pNext = &vulkan13Features;
        enabledFeatures.features.fragmentStoresAndAtomics = VK_TRUE;
        enabledFeatures.features.vertexPipelineStoresAndAtomics = VK_TRUE;
        vulkan12Features.bufferDeviceAddress = VK_TRUE;
        vulkan12Features.descriptorIndexing = VK_TRUE;
        vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
        vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
        vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        vulkan12Features.runtimeDescriptorArray = VK_TRUE;
        vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        vulkan13Features.synchronization2 = VK_TRUE;
        vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
        vulkan12Features.timelineSemaphore = VK_TRUE;
        vulkan13Features.dynamicRendering = VK_TRUE;

        vk::DeviceCreateInfo createInfo;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledExtensionCount = 1;
        const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        createInfo.ppEnabledExtensionNames = deviceExtensions;
        createInfo.pNext = &enabledFeatures;

        const auto deviceResult = m_PhysicalDevice.createDevice(createInfo);
        m_Device = deviceResult.value;
        if (!m_Device || deviceResult.result != vk::Result::eSuccess)
        {
            return;
        }

        for (const auto queueType : requestedQueues)
        {
            const auto queueFamilyIndex = queueFamilyIndices[GetQueueSlot(queueType)];
            if (!queueFamilyIndex.has_value())
            {
                continue;
            }

            std::unique_ptr<RHIQueue> queue(new RHIQueue(
                this,
                queueType,
                queueFamilyIndex.value(),
                m_Device.getQueue(queueFamilyIndex.value(), 0),
                0));
            if (!queue || !queue->IsValid())
            {
                continue;
            }

            auto* queuePtr = m_Queues.Register(std::move(queue));
            m_QueueLookup[GetQueueSlot(queueType)] = queuePtr;

            switch (queueType)
            {
                case RHIQueueType::Graphics:
                    m_Capabilities.queueTypes = m_Capabilities.queueTypes | RHIQueueTypeFlagBits::Graphics;
                    break;
                case RHIQueueType::Compute:
                    m_Capabilities.queueTypes = m_Capabilities.queueTypes | RHIQueueTypeFlagBits::Compute;
                    break;
                case RHIQueueType::Transfer:
                    m_Capabilities.queueTypes = m_Capabilities.queueTypes | RHIQueueTypeFlagBits::Transfer;
                    break;
                case RHIQueueType::Present:
                    m_Capabilities.queueTypes = m_Capabilities.queueTypes | RHIQueueTypeFlagBits::Present;
                    break;
                default:
                    break;
            }
        }

        m_Allocator = std::make_unique<VulkanMemoryAllocator>(m_Instance, m_PhysicalDevice, m_Device, m_Capabilities);
        if (!m_Allocator || !m_Allocator->IsValid())
        {
            m_QueueLookup = {};
            m_Queues.Clear();
            m_Allocator.reset();
            m_Device.destroy();
            m_Device = VK_NULL_HANDLE;
            return;
        }

        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIDevice, ~RHIDeviceImpl)()
    {
        Release();
    }

    RHIQueue*RHI_VK_FUNC_IMPL(RHIDevice, GetQueue)(RHIQueueType type) const
    {
        return m_QueueLookup[GetQueueSlot(type)];
    }

    bool RHI_VK_FUNC_IMPL(RHIDevice, WaitIdle)()
    {
        if (!m_Device)
        {
            return false;
        }

        return m_Device.waitIdle() == vk::Result::eSuccess;
    }

    bool RHI_VK_FUNC_IMPL(RHIDevice, WaitSyncPoint)(RHISyncPoint* syncPoint,
                                                    uint64_t timeout)
    {
        if (!m_Device || !syncPoint->valid || !syncPoint->queue)
        {
            return false;
        }

        auto semaphore = syncPoint->queue->GetSignalSemaphore();

        vk::SemaphoreWaitInfo waitInfo{};
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &semaphore;
        waitInfo.pValues = &syncPoint->value;

        auto result = m_Device.waitSemaphores(&waitInfo, timeout);

        return result == vk::Result::eSuccess;
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, FlushDeferredDeletions)()
    {
        if (!m_Device)
        {
            DeletionQueue::Execute(ExtractDeletionQueue());
            return;
        }

        if (!WaitIdle())
        {
            return;
        }

        auto pendingOperations = ExtractDeletionQueue();
        DeletionQueue::Execute(std::move(pendingOperations));
    }

    RHICommandPool*RHI_VK_FUNC_IMPL(RHIDevice, CreateCommandPool)(const RHICommandPoolDesc& desc,
                                                                  bool isDetached)
    {
        auto* queue = GetQueue(desc.queueType);
        HZ_RHI_DEBUG_RETURN_NULL_IF(!queue);

        std::unique_ptr<RHICommandPool> commandPool(new RHICommandPool(this, m_Device, desc, queue->GetFamilyIndex()));
        if (!commandPool || !commandPool->IsValid())
        {
            return nullptr;
        }

        commandPool->m_IsDetached = isDetached;
        auto* commandPoolPtr = commandPool.get();
        if (!isDetached)
            RegisterCommandPool(std::move(commandPool));
        else
            commandPool.release();
        return commandPoolPtr;
    }

    RHICommandPool*RHI_VK_FUNC_IMPL(RHIDevice, CreateCommandPoolUniformQueue)(const RHICommandPoolDesc& desc,
                                                                              bool isDetached)
    {
        auto* queue = GetUniformQueue();
        HZ_RHI_DEBUG_RETURN_NULL_IF(!queue);

        std::unique_ptr<RHICommandPool> commandPool(new RHICommandPool(this, m_Device, desc, queue->GetFamilyIndex()));
        if (!commandPool || !commandPool->IsValid())
        {
            return nullptr;
        }

        commandPool->m_IsDetached = isDetached;
        auto* commandPoolPtr = commandPool.get();
        if (!isDetached)
            RegisterCommandPool(std::move(commandPool));
        else
            commandPool.release();
        return commandPoolPtr;
    }

    RHIBuffer*RHI_VK_FUNC_IMPL(RHIDevice, CreateBuffer)(const RHIBufferDesc& desc, bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Allocator || !m_Allocator->IsValid());

        std::unique_ptr<RHIBuffer> buffer(new RHIBuffer(this, m_Allocator.get(), desc));
        if (!buffer || !buffer->IsValid())
        {
            return nullptr;
        }

        buffer->m_IsDetached = isDetached;
        auto* bufferPtr = buffer.get();
        if (!isDetached)
            RegisterBuffer(std::move(buffer));
        else
            buffer.release();
        return bufferPtr;
    }

    RHIBufferView*RHI_VK_FUNC_IMPL(RHIDevice, CreateBufferView)(RHIBuffer* buffer,
                                                                const RHIBufferViewDesc& desc,
                                                                bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device || !buffer);

        std::unique_ptr<RHIBufferView> bufferView(new RHIBufferView(this, buffer, desc, isDetached));
        if (!bufferView || !bufferView->IsValid())
        {
            return nullptr;
        }

        auto* bufferViewPtr = bufferView.get();
        bufferView.release();
        return bufferViewPtr;
    }

    RHIImage*RHI_VK_FUNC_IMPL(RHIDevice, CreateImage)(const RHIImageDesc& desc,
                                                      bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Allocator || !m_Allocator->IsValid());

        std::unique_ptr<RHIImage> image(new RHIImage(this, m_Allocator.get(), desc));
        if (!image || !image->IsValid())
        {
            return nullptr;
        }

        image->m_IsDetached = isDetached;
        auto* imagePtr = image.get();
        if (!isDetached)
            RegisterImage(std::move(image));
        else
            image.release();
        return imagePtr;
    }

    RHIResourceHeap*RHI_VK_FUNC_IMPL(RHIDevice, CreateResourceHeap)(const RHIResourceHeapDesc& desc,
                                                                    bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device);

        std::unique_ptr<RHIResourceHeap> heap(new RHIResourceHeap(this, m_Device, desc));
        if (!heap || !heap->IsValid())
        {
            return nullptr;
        }

        heap->m_IsDetached = isDetached;
        auto* heapPtr = heap.get();
        if (!isDetached)
            RegisterResourceHeap(std::move(heap));
        else
            heap.release();
        return heapPtr;
    }

    RHIResourceLayout*RHI_VK_FUNC_IMPL(RHIDevice, CreateResourceLayout)(const RHIResourceLayoutDesc& desc,
                                                                        bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device);

        std::unique_ptr<RHIResourceLayout> layout(new RHIResourceLayout(this, m_Device, desc));
        if (!layout || !layout->IsValid())
        {
            return nullptr;
        }

        layout->m_IsDetached = isDetached;
        auto* layoutPtr = layout.get();
        if (!isDetached)
            RegisterResourceLayout(std::move(layout));
        else
            layout.release();
        return layoutPtr;
    }

    RHIResourceSignature*RHI_VK_FUNC_IMPL(RHIDevice, CreateResourceSignature)(const RHIResourceSignatureDesc& desc,
                                                                              bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device);

        std::unique_ptr<RHIResourceSignature> signature(new RHIResourceSignature(this, m_Device, desc));
        if (!signature || !signature->IsValid())
        {
            return nullptr;
        }

        signature->m_IsDetached = isDetached;
        auto* signaturePtr = signature.get();
        if (!isDetached)
            RegisterResourceSignature(std::move(signature));
        else
            signature.release();
        return signaturePtr;
    }

    RHISampler*RHI_VK_FUNC_IMPL(RHIDevice, CreateSampler)(const RHISamplerDesc& desc, bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device);

        std::unique_ptr<RHISampler> sampler(new RHISampler(this, m_Device, desc));
        if (!sampler || !sampler->IsValid())
        {
            return nullptr;
        }

        sampler->m_IsDetached = isDetached;
        auto* samplerPtr = sampler.get();
        if (!isDetached)
            RegisterSampler(std::move(sampler));
        else
            sampler.release();
        return samplerPtr;
    }

    RHIShader*RHI_VK_FUNC_IMPL(RHIDevice, CreateShader)(const RHIShaderDesc& desc, bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device);

        std::unique_ptr<RHIShader> shader(new RHIShader(this, m_Device, desc));
        if (!shader || !shader->IsValid())
        {
            return nullptr;
        }

        shader->m_IsDetached = isDetached;
        auto* shaderPtr = shader.get();
        if (!isDetached)
            RegisterShader(std::move(shader));
        else
            shader.release();
        return shaderPtr;
    }

    RHIGraphicsPipeline*RHI_VK_FUNC_IMPL(RHIDevice, CreateGraphicsPipeline)(const RHIGraphicsPipelineDesc& desc,
                                                                            bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device);

        std::unique_ptr<RHIGraphicsPipeline> pipeline(new RHIGraphicsPipeline(this, m_Device, desc));
        if (!pipeline || !pipeline->IsValid())
        {
            return nullptr;
        }

        pipeline->m_IsDetached = isDetached;
        auto* pipelinePtr = pipeline.get();
        if (!isDetached)
            RegisterGraphicsPipeline(std::move(pipeline));
        else
            pipeline.release();
        return pipelinePtr;
    }

    RHIComputePipeline*RHI_VK_FUNC_IMPL(RHIDevice, CreateComputePipeline)(const RHIComputePipelineDesc& desc,
                                                                          bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device);

        std::unique_ptr<RHIComputePipeline> pipeline(new RHIComputePipeline(this, m_Device, desc));
        if (!pipeline || !pipeline->IsValid())
        {
            return nullptr;
        }

        pipeline->m_IsDetached = isDetached;
        auto* pipelinePtr = pipeline.get();
        if (!isDetached)
            RegisterComputePipeline(std::move(pipeline));
        else
            pipeline.release();
        return pipelinePtr;
    }

    RHISwapchain*RHI_VK_FUNC_IMPL(RHIDevice, CreateSwapchain)(const RHISwapchainDesc& desc,
                                                              bool isDetached)
    {
        auto* presentQueue = GetQueue(RHIQueueType::Present);
        HZ_RHI_DEBUG_RETURN_NULL_IF(!presentQueue || !desc.surface);

        std::unique_ptr<RHISwapchain> swapchain(
            new RHISwapchain(m_PhysicalDevice, this, desc, presentQueue->GetFamilyIndex()));
        if (!swapchain || !swapchain->IsValid())
        {
            return nullptr;
        }

        swapchain->m_IsDetached = isDetached;
        auto* swapchainPtr = swapchain.get();
        if (!isDetached)
            RegisterSwapchain(std::move(swapchain));
        else
            swapchain.release();
        return swapchainPtr;
    }

    RHIImageView*RHI_VK_FUNC_IMPL(RHIDevice, CreateImageView)(RHIImage* image,
                                                              const RHIImageViewDesc& desc,
                                                              bool isDetached)
    {
        HZ_RHI_DEBUG_RETURN_NULL_IF(!m_Device || !image);

        std::unique_ptr<RHIImageView> imageView(new RHIImageView(this, image, desc, isDetached));
        if (!imageView || !imageView->IsValid())
        {
            return nullptr;
        }

        auto* imageViewPtr = imageView.get();
        imageView.release();
        return imageViewPtr;
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* instanceOwner = m_InstanceOwner;
        ReleaseWithoutUnregister();
        if (instanceOwner)
        {
            instanceOwner->UnregisterDevice(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, ReleaseWithoutUnregister)()
    {
        if (!m_IsValid)
        {
            return;
        }

        for (const auto& swapchain : m_Swapchains)
        {
            if (swapchain)
            {
                swapchain->ReleaseWithoutUnregister();
            }
        }
        m_Swapchains.Clear();

        for (const auto& commandPool : m_CommandPools)
        {
            if (commandPool)
            {
                commandPool->ReleaseWithoutUnregister();
            }
        }
        m_CommandPools.Clear();

        for (const auto& image : m_Images)
        {
            if (image)
            {
                image->ReleaseWithoutUnregister();
            }
        }
        m_Images.Clear();

        for (const auto& computePipeline : m_ComputePipelines)
        {
            if (computePipeline)
            {
                computePipeline->ReleaseWithoutUnregister();
            }
        }
        m_ComputePipelines.Clear();

        for (const auto& graphicsPipeline : m_GraphicsPipelines)
        {
            if (graphicsPipeline)
            {
                graphicsPipeline->ReleaseWithoutUnregister();
            }
        }
        m_GraphicsPipelines.Clear();

        for (const auto& buffer : m_Buffers)
        {
            if (buffer)
            {
                buffer->ReleaseWithoutUnregister();
            }
        }
        m_Buffers.Clear();

        for (const auto& resourceHeap : m_ResourceHeaps)
        {
            if (resourceHeap)
            {
                resourceHeap->ReleaseWithoutUnregister();
            }
        }
        m_ResourceHeaps.Clear();

        for (const auto& resourceLayout : m_ResourceLayouts)
        {
            if (resourceLayout)
            {
                resourceLayout->ReleaseWithoutUnregister();
            }
        }
        m_ResourceLayouts.Clear();

        for (const auto& resourceSignature : m_ResourceSignatures)
        {
            if (resourceSignature)
            {
                resourceSignature->ReleaseWithoutUnregister();
            }
        }
        m_ResourceSignatures.Clear();

        for (const auto& sampler : m_Samplers)
        {
            if (sampler)
            {
                sampler->ReleaseWithoutUnregister();
            }
        }
        m_Samplers.Clear();

        for (const auto& shader : m_Shaders)
        {
            if (shader)
            {
                shader->ReleaseWithoutUnregister();
            }
        }
        m_Shaders.Clear();

        m_QueueLookup = {};
        m_Queues.Clear();

        auto pendingOperations = ExtractDeletionQueue();
        const auto allocatorHandle = m_Allocator ? m_Allocator->Detach() : VK_NULL_HANDLE;
        m_Allocator.reset();

        const auto device = m_Device;
        if (m_InstanceOwner)
        {
            m_InstanceOwner->EnqueueDeletion(
                [device, allocatorHandle, pendingOperations = std::move(pendingOperations)]() mutable {
                    if (!device)
                    {
                        return;
                    }

                    (void)device.waitIdle();
                    DeletionQueue::Execute(std::move(pendingOperations));
                    VulkanMemoryAllocator::Destroy(allocatorHandle);
                    device.destroy();
                });
        }
        else if (device)
        {
            (void)device.waitIdle();
            DeletionQueue::Execute(std::move(pendingOperations));
            VulkanMemoryAllocator::Destroy(allocatorHandle);
            device.destroy();
        }

        m_Device = VK_NULL_HANDLE;
        m_IsValid = false;
        m_InstanceOwner = nullptr;
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterSwapchain)(std::unique_ptr<RHISwapchain> swapchain)
    {
        m_Swapchains.Register(std::move(swapchain));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterSwapchain)(RHISwapchain* swapchain)
    {
        m_Swapchains.Unregister(swapchain);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterCommandPool)(std::unique_ptr<RHICommandPool> commandPool)
    {
        m_CommandPools.Register(std::move(commandPool));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterCommandPool)(RHICommandPool* commandPool)
    {
        m_CommandPools.Unregister(commandPool);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterBuffer)(std::unique_ptr<RHIBuffer> buffer)
    {
        m_Buffers.Register(std::move(buffer));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterBuffer)(RHIBuffer* buffer)
    {
        m_Buffers.Unregister(buffer);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterImage)(std::unique_ptr<RHIImage> image)
    {
        m_Images.Register(std::move(image));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterImage)(RHIImage* image)
    {
        m_Images.Unregister(image);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterComputePipeline)(std::unique_ptr<RHIComputePipeline> pipeline)
    {
        m_ComputePipelines.Register(std::move(pipeline));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterComputePipeline)(RHIComputePipeline* pipeline)
    {
        m_ComputePipelines.Unregister(pipeline);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterGraphicsPipeline)(std::unique_ptr<RHIGraphicsPipeline> pipeline)
    {
        m_GraphicsPipelines.Register(std::move(pipeline));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterGraphicsPipeline)(RHIGraphicsPipeline* pipeline)
    {
        m_GraphicsPipelines.Unregister(pipeline);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterResourceHeap)(std::unique_ptr<RHIResourceHeap> heap)
    {
        m_ResourceHeaps.Register(std::move(heap));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterResourceHeap)(RHIResourceHeap* heap)
    {
        m_ResourceHeaps.Unregister(heap);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterResourceLayout)(std::unique_ptr<RHIResourceLayout> layout)
    {
        m_ResourceLayouts.Register(std::move(layout));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterResourceLayout)(RHIResourceLayout* layout)
    {
        m_ResourceLayouts.Unregister(layout);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterResourceSignature)(std::unique_ptr<RHIResourceSignature> signature)
    {
        m_ResourceSignatures.Register(std::move(signature));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterResourceSignature)(RHIResourceSignature* signature)
    {
        m_ResourceSignatures.Unregister(signature);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterSampler)(std::unique_ptr<RHISampler> sampler)
    {
        m_Samplers.Register(std::move(sampler));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterSampler)(RHISampler* sampler)
    {
        m_Samplers.Unregister(sampler);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterShader)(std::unique_ptr<RHIShader> shader)
    {
        m_Shaders.Register(std::move(shader));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterShader)(RHIShader* shader)
    {
        m_Shaders.Unregister(shader);
    }

    RHI_VK_FUNC_IMPL(RHIDevice, RHIDeviceImpl)(RHIDevice&& device) noexcept
    {
        m_InstanceOwner = device.m_InstanceOwner;
        m_Adapter = device.m_Adapter;
        m_Capabilities = device.m_Capabilities;
        m_DeletionQueue = std::move(device.m_DeletionQueue);
        m_Instance = device.m_Instance;
        m_PhysicalDevice = device.m_PhysicalDevice;
        m_Device = device.m_Device;
        m_QueueLookup = device.m_QueueLookup;
        m_Queues = std::move(device.m_Queues);
        m_Allocator = std::move(device.m_Allocator);
        m_CommandPools = std::move(device.m_CommandPools);
        m_Buffers = std::move(device.m_Buffers);
        m_ComputePipelines = std::move(device.m_ComputePipelines);
        m_GraphicsPipelines = std::move(device.m_GraphicsPipelines);
        m_Images = std::move(device.m_Images);
        m_ResourceHeaps = std::move(device.m_ResourceHeaps);
        m_ResourceLayouts = std::move(device.m_ResourceLayouts);
        m_ResourceSignatures = std::move(device.m_ResourceSignatures);
        m_Samplers = std::move(device.m_Samplers);
        m_Shaders = std::move(device.m_Shaders);
        m_Swapchains = std::move(device.m_Swapchains);
        m_IsValid = device.m_IsValid;
    }

    size_t RHI_VK_FUNC_IMPL(RHIDevice, GetQueueSlot)(RHIQueueType type)
    {
        switch (type)
        {
            case RHIQueueType::Graphics:
                return 0;
            case RHIQueueType::Compute:
                return 1;
            case RHIQueueType::Transfer:
                return 2;
            case RHIQueueType::Present:
                return 3;
        }

        return 0;
    }
} // namespace Hazel