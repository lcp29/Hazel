//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    struct VulkanRequiredFeatures
    {
        bool supportDynamicRendering = false;
        bool supportDescriptorIndexing = false;
        bool supportDescriptorPartiallyBound = false;
        bool supportGpuAddress = false;
        bool supportTimelineSemaphore = false;
        bool supportSynchronization2 = false;
        bool supportFragmentStoresAndAtomics = false;
        bool supportVertexPipelineStoresAndAtomics = false;
        bool supportDescriptorBindingVariableDescriptorCount = false;
        bool supportDescriptorBindingStorageBufferUpdateAfterBind = false;
        bool supportDescriptorBindingSampledImageUpdateAfterBind = false;
        bool supportRuntimeDescriptorArray = false;
        bool supportShaderSampledImageArrayNonUniformIndexing = false;
    };

    RHI_VK_CLASS_IMPL(RHIAdapter)
    {
      public:
        bool CanCreateDevice(const RHIDeviceCapabilities& caps) const;

        const RHIDeviceCapabilities& GetCapabilities() const { return m_Capabilities; }

        const std::string& GetName() const { return m_Info.name; }

        uint32_t GetDeviceId() const { return m_Info.deviceId; }

        uint32_t GetVendorId() const { return m_Info.vendorId; }

        RHIAdapterType GetType() const { return m_Info.type; }

        RHIAdapterImpl() = default;
        explicit RHIAdapterImpl(vk::PhysicalDevice adapter);

        vk::PhysicalDevice GetHandle() const { return m_Adapter; }

      private:
        RHIAdapterInfo m_Info;
        RHIDeviceCapabilities m_Capabilities;
        vk::PhysicalDevice m_Adapter;
        vk::PhysicalDeviceProperties m_Properties;
        VulkanRequiredFeatures m_RequiredVulkanFeatures;
    };
} // namespace Hazel