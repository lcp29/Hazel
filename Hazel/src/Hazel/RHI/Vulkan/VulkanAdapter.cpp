//
// Created by helmholtz on 2026/3/14.
//


#include <vulkan/vulkan.hpp>

#include "../RHIAdapter.h"
#include "../RHIBase.h"
#include "VulkanCommon.h"
#include "VulkanAdapter.h"

#include <cstring>

namespace Hazel
{
    bool RHI_VK_FUNC_IMPL(RHIAdapter, CanCreateDevice)(const RHIDeviceCapabilities &caps) const
    {
        if (m_Properties.apiVersion < vk::ApiVersion13)
        {
            return false;
        }

        if ((caps.queueTypes & RHIQueueTypeFlagBits::Graphics)
            && !(m_Capabilities.queueTypes & RHIQueueTypeFlagBits::Graphics))
        {
            return false;
        }

        if ((caps.queueTypes & RHIQueueTypeFlagBits::Compute)
            && !(m_Capabilities.queueTypes & RHIQueueTypeFlagBits::Compute))
        {
            return false;
        }

        if ((caps.queueTypes & RHIQueueTypeFlagBits::Transfer)
            && !(m_Capabilities.queueTypes & RHIQueueTypeFlagBits::Transfer))
        {
            return false;
        }

        if (caps.supportSubgroup)
        {
            if (!m_Capabilities.supportSubgroup)
            {
                return false;
            }
            // we are not checking subgroup size range but just getting it
        }

        // required features supported by almost all devices
        if (!(m_RequiredVulkanFeatures.supportDescriptorIndexing &&
              m_RequiredVulkanFeatures.supportDynamicRendering &&
              m_RequiredVulkanFeatures.supportGpuAddress &&
              m_RequiredVulkanFeatures.supportDescriptorPartiallyBound &&
              m_RequiredVulkanFeatures.supportTimelineSemaphore &&
              m_RequiredVulkanFeatures.supportSynchronization2 &&
              m_RequiredVulkanFeatures.supportFragmentStoresAndAtomics &&
              m_RequiredVulkanFeatures.supportVertexPipelineStoresAndAtomics))
        {
            return false;
        }

        return true;
    }

    RHI_VK_FUNC_IMPL(RHIAdapter, RHIAdapterImpl)(vk::PhysicalDevice adapter)
    {
        m_Adapter = adapter;

        m_Properties = adapter.getProperties();
        const auto extensionProperties = adapter.enumerateDeviceExtensionProperties();
        const auto queueFamilyProperties = adapter.getQueueFamilyProperties();

        const auto hasExtension = [&extensionProperties](const char *extensionName)
        {
            for (const auto &extensionProperty: extensionProperties)
            {
                if (std::strcmp(extensionProperty.extensionName, extensionName) == 0)
                {
                    return true;
                }
            }
            return false;
        };

        const bool supportsVulkan11 = m_Properties.apiVersion >= VK_API_VERSION_1_1;
        const bool supportsVulkan12 = m_Properties.apiVersion >= VK_API_VERSION_1_2;
        const bool supportsVulkan13 = m_Properties.apiVersion >= VK_API_VERSION_1_3;

        vk::PhysicalDeviceFeatures2 features2;
        vk::PhysicalDeviceVulkan11Features vulkan11Features;
        vk::PhysicalDeviceVulkan12Features vulkan12Features;
        vk::PhysicalDeviceVulkan13Features vulkan13Features;
        features2.pNext = &vulkan11Features;
        vulkan11Features.pNext = &vulkan12Features;
        vulkan12Features.pNext = &vulkan13Features;
        adapter.getFeatures2(&features2);

        vk::PhysicalDeviceSubgroupProperties subgroupProperties;
        vk::PhysicalDeviceProperties2 properties2;
        properties2.pNext = &subgroupProperties;
        adapter.getProperties2(&properties2);

        m_Info.name = m_Properties.deviceName.data();
        m_Info.deviceId = m_Properties.deviceID;
        m_Info.vendorId = m_Properties.vendorID;
        m_Info.type = VulkanConvertAdapterType(m_Properties.deviceType);

        for (const auto &queueFamily: queueFamilyProperties)
        {
            if (queueFamily.queueCount == 0)
            {
                continue;
            }

            if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics)
            {
                m_Capabilities.queueTypes = m_Capabilities.queueTypes | RHIQueueTypeFlagBits::Graphics;
            }
            if ((queueFamily.queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute)
            {
                m_Capabilities.queueTypes = m_Capabilities.queueTypes | RHIQueueTypeFlagBits::Compute;
            }
            if ((queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) == vk::QueueFlagBits::eTransfer)
            {
                m_Capabilities.queueTypes = m_Capabilities.queueTypes | RHIQueueTypeFlagBits::Transfer;
            }
        }

        m_Capabilities.supportSubgroup = supportsVulkan11 && subgroupProperties.subgroupSize > 0;
        m_Capabilities.subgroupSizeMin = subgroupProperties.subgroupSize;
        m_Capabilities.subgroupSizeMax = subgroupProperties.subgroupSize;

        m_RequiredVulkanFeatures.supportGpuAddress =
                supportsVulkan12 && vulkan12Features.bufferDeviceAddress;
        m_RequiredVulkanFeatures.supportDescriptorIndexing =
                supportsVulkan12 && vulkan12Features.descriptorIndexing;
        m_RequiredVulkanFeatures.supportDynamicRendering = supportsVulkan13 && vulkan13Features.dynamicRendering;
        m_RequiredVulkanFeatures.supportDescriptorPartiallyBound =
                m_RequiredVulkanFeatures.supportDescriptorIndexing && vulkan12Features.descriptorBindingPartiallyBound;
        m_RequiredVulkanFeatures.supportTimelineSemaphore =
                supportsVulkan12 && vulkan12Features.timelineSemaphore;
        m_RequiredVulkanFeatures.supportSynchronization2 =
                supportsVulkan13 && vulkan13Features.synchronization2;
        m_RequiredVulkanFeatures.supportFragmentStoresAndAtomics = features2.features.fragmentStoresAndAtomics;
        m_RequiredVulkanFeatures.supportVertexPipelineStoresAndAtomics = features2.features.
                vertexPipelineStoresAndAtomics;
    }
} // Hazel
