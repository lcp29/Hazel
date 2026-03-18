//
// Created by helmholtz on 2026/3/14.
//

#define VULKAN_HPP_NO_EXCEPTIONS

#include <set>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "../RHIBase.h"
#include "../RHIDevice.h"
#include "VulkanBuffer.h"
#include "VulkanBufferView.h"
#include "VulkanCommandPool.h"
#include "VulkanComputePipeline.h"
#include "VulkanDevice.h"
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

namespace Hazel
{
    namespace
    {
        constexpr float s_QueuePriority = 1.0f;

        bool QueueFlagsMatch(vk::QueueFlags flags, vk::QueueFlagBits bit)
        {
            return (flags & bit) == bit;
        }

        std::optional<uint32_t> FindQueueFamilyIndex(const std::vector<vk::QueueFamilyProperties> &queueFamilies,
                                                     RHIQueueType type)
        {
            const auto matches = [type](const vk::QueueFamilyProperties &queueFamily)
            {
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

            const auto preferred = [type](const vk::QueueFamilyProperties &queueFamily)
            {
                switch (type)
                {
                    case RHIQueueType::Graphics:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eGraphics);
                    case RHIQueueType::Compute:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eCompute)
                               && !QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eGraphics);
                    case RHIQueueType::Transfer:
                        return QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eTransfer)
                               && !QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eGraphics)
                               && !QueueFlagsMatch(queueFamily.queueFlags, vk::QueueFlagBits::eCompute);
                    case RHIQueueType::Present:
                        return false;
                }

                return false;
            };

            for (uint32_t i = 0; i < queueFamilies.size(); i++)
            {
                if (queueFamilies[i].queueCount > 0 && preferred(queueFamilies[i]))
                {
                    return i;
                }
            }

            for (uint32_t i = 0; i < queueFamilies.size(); i++)
            {
                if (queueFamilies[i].queueCount > 0 && matches(queueFamilies[i]))
                {
                    return i;
                }
            }

            return std::nullopt;
        }

        std::optional<uint32_t> FindPresentQueueFamilyIndex(vk::PhysicalDevice physicalDevice,
                                                            const std::vector<vk::QueueFamilyProperties> &queueFamilies,
                                                            vk::SurfaceKHR surface)
        {
            for (uint32_t i = 0; i < queueFamilies.size(); i++)
            {
                if (queueFamilies[i].queueCount == 0)
                {
                    continue;
                }

                if (physicalDevice.getSurfaceSupportKHR(i, surface).value)
                {
                    return i;
                }
            }

            return std::nullopt;
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHIDevice, RHIDeviceImpl)(RHIInstance *instanceOwner,
                                               vk::Instance instance,
                                               const RHIAdapter &adapter,
                                               const RHIDeviceCapabilities &caps,
                                               const RHISurface *surface)
        : m_InstanceOwner(instanceOwner), m_Adapter(adapter), m_Capabilities(caps), m_Instance(instance)
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
        for (const auto queueType: requestedQueues)
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
        for (const auto &queueFamilyIndex: queueFamilyIndices)
        {
            if (queueFamilyIndex.has_value())
            {
                uniqueQueueFamilies.insert(queueFamilyIndex.value());
            }
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(uniqueQueueFamilies.size());
        for (const auto queueFamilyIndex: uniqueQueueFamilies)
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
        vulkan12Features.bufferDeviceAddress = VK_TRUE;
        vulkan12Features.descriptorIndexing = VK_TRUE;
        vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
        vulkan12Features.timelineSemaphore = VK_TRUE;
        vulkan13Features.dynamicRendering = VK_TRUE;

        vk::DeviceCreateInfo createInfo;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledExtensionCount = 1;
        const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        createInfo.ppEnabledExtensionNames = deviceExtensions;
        createInfo.pNext = &enabledFeatures;

        const auto deviceResult = m_PhysicalDevice.createDevice(createInfo);
        m_Device = deviceResult.value;
        if (!m_Device || deviceResult.result != vk::Result::eSuccess)
        {
            return;
        }

        for (const auto queueType: requestedQueues)
        {
            const auto queueFamilyIndex = queueFamilyIndices[GetQueueSlot(queueType)];
            if (!queueFamilyIndex.has_value())
            {
                continue;
            }

            std::unique_ptr<RHIQueue> queue(new RHIQueue(this,
                                                         queueType,
                                                         queueFamilyIndex.value(),
                                                         m_Device.getQueue(queueFamilyIndex.value(), 0),
                                                         0));
            if (!queue || !queue->IsValid())
            {
                continue;
            }

            auto *queuePtr = RegisterOwnedObject(m_Queues, std::move(queue));
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
            }
        }

        m_Allocator = std::make_unique<VulkanMemoryAllocator>(m_Instance, m_PhysicalDevice, m_Device, m_Capabilities);
        if (!m_Allocator || !m_Allocator->IsValid())
        {
            m_QueueLookup = {};
            m_Queues.clear();
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

    RHIQueue *RHI_VK_FUNC_IMPL(RHIDevice, GetQueue)(RHIQueueType type) const
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

    RHICommandPool *RHI_VK_FUNC_IMPL(RHIDevice, CreateCommandPool)(const RHICommandPoolDesc &desc)
    {
        auto *queue = GetQueue(desc.queueType);
        if (!queue)
        {
            return nullptr;
        }

        std::unique_ptr<RHICommandPool> commandPool(new RHICommandPool(this, m_Device, desc, queue->GetFamilyIndex()));
        if (!commandPool || !commandPool->IsValid())
        {
            return nullptr;
        }

        auto *commandPoolPtr = commandPool.get();
        RegisterCommandPool(std::move(commandPool));
        return commandPoolPtr;
    }

    RHIBuffer *RHI_VK_FUNC_IMPL(RHIDevice, CreateBuffer)(const RHIBufferDesc &desc)
    {
        if (!m_Allocator || !m_Allocator->IsValid())
        {
            return nullptr;
        }

        std::unique_ptr<RHIBuffer> buffer(new RHIBuffer(this, m_Allocator.get(), desc));
        if (!buffer || !buffer->IsValid())
        {
            return nullptr;
        }

        auto *bufferPtr = buffer.get();
        RegisterBuffer(std::move(buffer));
        return bufferPtr;
    }

    RHIBufferView *RHI_VK_FUNC_IMPL(RHIDevice, CreateBufferView)(RHIBuffer *buffer, const RHIBufferViewDesc &desc)
    {
        if (!m_Device || !buffer)
        {
            return nullptr;
        }

        std::unique_ptr<RHIBufferView> bufferView(new RHIBufferView(this, buffer, desc));
        if (!bufferView || !bufferView->IsValid())
        {
            return nullptr;
        }

        auto *bufferViewPtr = bufferView.get();
        bufferView.release();
        return bufferViewPtr;
    }

    RHIImage *RHI_VK_FUNC_IMPL(RHIDevice, CreateImage)(const RHIImageDesc &desc)
    {
        if (!m_Allocator || !m_Allocator->IsValid())
        {
            return nullptr;
        }

        std::unique_ptr<RHIImage> image(new RHIImage(this, m_Allocator.get(), desc));
        if (!image || !image->IsValid())
        {
            return nullptr;
        }

        auto *imagePtr = image.get();
        RegisterImage(std::move(image));
        return imagePtr;
    }

    RHIResourceHeap *RHI_VK_FUNC_IMPL(RHIDevice, CreateResourceHeap)(const RHIResourceHeapDesc &desc)
    {
        if (!m_Device)
        {
            return nullptr;
        }

        std::unique_ptr<RHIResourceHeap> heap(new RHIResourceHeap(this, m_Device, desc));
        if (!heap || !heap->IsValid())
        {
            return nullptr;
        }

        auto *heapPtr = heap.get();
        RegisterResourceHeap(std::move(heap));
        return heapPtr;
    }

    RHIResourceLayout *RHI_VK_FUNC_IMPL(RHIDevice, CreateResourceLayout)(const RHIResourceLayoutDesc &desc)
    {
        if (!m_Device)
        {
            return nullptr;
        }

        std::unique_ptr<RHIResourceLayout> layout(new RHIResourceLayout(this, m_Device, desc));
        if (!layout || !layout->IsValid())
        {
            return nullptr;
        }

        auto *layoutPtr = layout.get();
        RegisterResourceLayout(std::move(layout));
        return layoutPtr;
    }

    RHIResourceSignature *RHI_VK_FUNC_IMPL(RHIDevice, CreateResourceSignature)(const RHIResourceSignatureDesc &desc)
    {
        if (!m_Device)
        {
            return nullptr;
        }

        std::unique_ptr<RHIResourceSignature> signature(new RHIResourceSignature(this, m_Device, desc));
        if (!signature || !signature->IsValid())
        {
            return nullptr;
        }

        auto *signaturePtr = signature.get();
        RegisterResourceSignature(std::move(signature));
        return signaturePtr;
    }

    RHISampler *RHI_VK_FUNC_IMPL(RHIDevice, CreateSampler)(const RHISamplerDesc &desc)
    {
        if (!m_Device)
        {
            return nullptr;
        }

        std::unique_ptr<RHISampler> sampler(new RHISampler(this, m_Device, desc));
        if (!sampler || !sampler->IsValid())
        {
            return nullptr;
        }

        auto *samplerPtr = sampler.get();
        RegisterSampler(std::move(sampler));
        return samplerPtr;
    }

    RHIShader *RHI_VK_FUNC_IMPL(RHIDevice, CreateShader)(const RHIShaderDesc &desc)
    {
        if (!m_Device)
        {
            return nullptr;
        }

        std::unique_ptr<RHIShader> shader(new RHIShader(this, m_Device, desc));
        if (!shader || !shader->IsValid())
        {
            return nullptr;
        }

        auto *shaderPtr = shader.get();
        RegisterShader(std::move(shader));
        return shaderPtr;
    }

    RHIGraphicsPipeline *RHI_VK_FUNC_IMPL(RHIDevice, CreateGraphicsPipeline)(const RHIGraphicsPipelineDesc &desc)
    {
        if (!m_Device)
        {
            return nullptr;
        }

        std::unique_ptr<RHIGraphicsPipeline> pipeline(new RHIGraphicsPipeline(this, m_Device, desc));
        if (!pipeline || !pipeline->IsValid())
        {
            return nullptr;
        }

        auto *pipelinePtr = pipeline.get();
        RegisterGraphicsPipeline(std::move(pipeline));
        return pipelinePtr;
    }

    RHIComputePipeline *RHI_VK_FUNC_IMPL(RHIDevice, CreateComputePipeline)(const RHIComputePipelineDesc &desc)
    {
        if (!m_Device)
        {
            return nullptr;
        }

        std::unique_ptr<RHIComputePipeline> pipeline(new RHIComputePipeline(this, m_Device, desc));
        if (!pipeline || !pipeline->IsValid())
        {
            return nullptr;
        }

        auto *pipelinePtr = pipeline.get();
        RegisterComputePipeline(std::move(pipeline));
        return pipelinePtr;
    }

    RHISwapchain *RHI_VK_FUNC_IMPL(RHIDevice, CreateSwapchain)(const RHISwapchainDesc &desc)
    {
        auto *presentQueue = GetQueue(RHIQueueType::Present);
        if (!presentQueue || !desc.surface)
        {
            return nullptr;
        }

        std::unique_ptr<RHISwapchain> swapchain(new RHISwapchain(m_PhysicalDevice,
                                                                 this,
                                                                 desc,
                                                                 presentQueue->GetFamilyIndex()));
        if (!swapchain || !swapchain->IsValid())
        {
            return nullptr;
        }

        auto *swapchainPtr = swapchain.get();
        RegisterSwapchain(std::move(swapchain));
        return swapchainPtr;
    }

    RHIImageView *RHI_VK_FUNC_IMPL(RHIDevice, CreateImageView)(RHIImage *image, const RHIImageViewDesc &desc)
    {
        if (!m_Device || !image)
        {
            return nullptr;
        }

        std::unique_ptr<RHIImageView> imageView(new RHIImageView(this, image, desc));
        if (!imageView || !imageView->IsValid())
        {
            return nullptr;
        }

        auto *imageViewPtr = imageView.get();
        imageView.release();
        return imageViewPtr;
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *instanceOwner = m_InstanceOwner;
        ReleaseFromOwner();
        if (instanceOwner)
        {
            instanceOwner->UnregisterDevice(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, ReleaseFromOwner)()
    {
        if (!m_IsValid)
        {
            return;
        }

        for (const auto &swapchain: m_Swapchains)
        {
            if (swapchain)
            {
                swapchain->ReleaseWithoutUnregister();
            }
        }
        m_Swapchains.clear();

        for (const auto &commandPool: m_CommandPools)
        {
            if (commandPool)
            {
                commandPool->ReleaseWithoutUnregister();
            }
        }
        m_CommandPools.clear();

        for (const auto &image: m_Images)
        {
            if (image)
            {
                image->ReleaseWithoutUnregister();
            }
        }
        m_Images.clear();

        for (const auto &computePipeline: m_ComputePipelines)
        {
            if (computePipeline)
            {
                computePipeline->ReleaseWithoutUnregister();
            }
        }
        m_ComputePipelines.clear();

        for (const auto &graphicsPipeline: m_GraphicsPipelines)
        {
            if (graphicsPipeline)
            {
                graphicsPipeline->ReleaseWithoutUnregister();
            }
        }
        m_GraphicsPipelines.clear();

        for (const auto &buffer: m_Buffers)
        {
            if (buffer)
            {
                buffer->ReleaseWithoutUnregister();
            }
        }
        m_Buffers.clear();

        for (const auto &resourceHeap: m_ResourceHeaps)
        {
            if (resourceHeap)
            {
                resourceHeap->ReleaseWithoutUnregister();
            }
        }
        m_ResourceHeaps.clear();

        for (const auto &resourceLayout: m_ResourceLayouts)
        {
            if (resourceLayout)
            {
                resourceLayout->ReleaseWithoutUnregister();
            }
        }
        m_ResourceLayouts.clear();

        for (const auto &resourceSignature: m_ResourceSignatures)
        {
            if (resourceSignature)
            {
                resourceSignature->ReleaseWithoutUnregister();
            }
        }
        m_ResourceSignatures.clear();

        for (const auto &sampler: m_Samplers)
        {
            if (sampler)
            {
                sampler->ReleaseWithoutUnregister();
            }
        }
        m_Samplers.clear();

        for (const auto &shader: m_Shaders)
        {
            if (shader)
            {
                shader->ReleaseWithoutUnregister();
            }
        }
        m_Shaders.clear();

        m_QueueLookup = {};
        m_Queues.clear();

        auto pendingOperations = ExtractDeletionQueue();
        const auto allocatorHandle = m_Allocator ? m_Allocator->Detach() : VK_NULL_HANDLE;
        m_Allocator.reset();

        const auto device = m_Device;
        if (m_InstanceOwner)
        {
            m_InstanceOwner->EnqueueDeletion(
                [device, allocatorHandle, pendingOperations = std::move(pendingOperations)]() mutable
                {
                    if (!device)
                    {
                        return;
                    }

                    (void) device.waitIdle();
                    DeletionQueue::Execute(std::move(pendingOperations));
                    VulkanMemoryAllocator::Destroy(allocatorHandle);
                    device.destroy();
                });
        }
        else if (device)
        {
            (void) device.waitIdle();
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
        RegisterOwnedObject(m_Swapchains, std::move(swapchain));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterSwapchain)(RHISwapchain *swapchain)
    {
        UnregisterOwnedObject(m_Swapchains, swapchain);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterCommandPool)(std::unique_ptr<RHICommandPool> commandPool)
    {
        RegisterOwnedObject(m_CommandPools, std::move(commandPool));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterCommandPool)(RHICommandPool *commandPool)
    {
        UnregisterOwnedObject(m_CommandPools, commandPool);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterBuffer)(std::unique_ptr<RHIBuffer> buffer)
    {
        RegisterOwnedObject(m_Buffers, std::move(buffer));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterBuffer)(RHIBuffer *buffer)
    {
        UnregisterOwnedObject(m_Buffers, buffer);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterImage)(std::unique_ptr<RHIImage> image)
    {
        RegisterOwnedObject(m_Images, std::move(image));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterImage)(RHIImage *image)
    {
        UnregisterOwnedObject(m_Images, image);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterComputePipeline)(std::unique_ptr<RHIComputePipeline> pipeline)
    {
        RegisterOwnedObject(m_ComputePipelines, std::move(pipeline));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterComputePipeline)(RHIComputePipeline *pipeline)
    {
        UnregisterOwnedObject(m_ComputePipelines, pipeline);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterGraphicsPipeline)(std::unique_ptr<RHIGraphicsPipeline> pipeline)
    {
        RegisterOwnedObject(m_GraphicsPipelines, std::move(pipeline));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterGraphicsPipeline)(RHIGraphicsPipeline *pipeline)
    {
        UnregisterOwnedObject(m_GraphicsPipelines, pipeline);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterResourceHeap)(std::unique_ptr<RHIResourceHeap> heap)
    {
        RegisterOwnedObject(m_ResourceHeaps, std::move(heap));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterResourceHeap)(RHIResourceHeap *heap)
    {
        UnregisterOwnedObject(m_ResourceHeaps, heap);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterResourceLayout)(std::unique_ptr<RHIResourceLayout> layout)
    {
        RegisterOwnedObject(m_ResourceLayouts, std::move(layout));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterResourceLayout)(RHIResourceLayout *layout)
    {
        UnregisterOwnedObject(m_ResourceLayouts, layout);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterResourceSignature)(std::unique_ptr<RHIResourceSignature> signature)
    {
        RegisterOwnedObject(m_ResourceSignatures, std::move(signature));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterResourceSignature)(RHIResourceSignature *signature)
    {
        UnregisterOwnedObject(m_ResourceSignatures, signature);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterSampler)(std::unique_ptr<RHISampler> sampler)
    {
        RegisterOwnedObject(m_Samplers, std::move(sampler));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterSampler)(RHISampler *sampler)
    {
        UnregisterOwnedObject(m_Samplers, sampler);
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, RegisterShader)(std::unique_ptr<RHIShader> shader)
    {
        RegisterOwnedObject(m_Shaders, std::move(shader));
    }

    void RHI_VK_FUNC_IMPL(RHIDevice, UnregisterShader)(RHIShader *shader)
    {
        UnregisterOwnedObject(m_Shaders, shader);
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
