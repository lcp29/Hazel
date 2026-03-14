//
// Created by helmholtz on 2026/3/14.
//

#pragma once
#include "../RHIAdapter.h"
#include "../RHICommon.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    inline RHIAdapterType VulkanConvertAdapterType(vk::PhysicalDeviceType type)
    {
        switch (type)
        {
            case vk::PhysicalDeviceType::eCpu:
                return RHIAdapterType::CPU;
            case vk::PhysicalDeviceType::eIntegratedGpu:
                return RHIAdapterType::IntegratedGPU;
            case vk::PhysicalDeviceType::eDiscreteGpu:
                return RHIAdapterType::DiscreteGPU;

        }
    }

    inline VkDebugUtilsMessageSeverityFlagsEXT VulkanConvertDebugMessageSeverity(DebugMessageSeverity severity)
    {
        VkDebugUtilsMessageSeverityFlagsEXT flags = 0;
        if (severity & DebugMessageSeverityFlagBits::Verbose)
        {
            flags |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        } else if (severity & DebugMessageSeverityFlagBits::Info)
        {
            flags |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        } else if (severity & DebugMessageSeverityFlagBits::Warning)
        {
            flags |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        } else if (severity & DebugMessageSeverityFlagBits::Error)
        {
            flags |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        }
        return flags;
    }

    inline VkDebugUtilsMessageTypeFlagsEXT VulkanConvertDebugMessageType(DebugMessageType type)
    {
        VkDebugUtilsMessageTypeFlagsEXT flags = 0;
        if (type & DebugMessageTypeFlagBits::General)
        {
            flags |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        } else if (type & DebugMessageTypeFlagBits::Performance)
        {
            flags |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        } else if (type & DebugMessageTypeFlagBits::Validation)
        {
            flags |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        }
        return flags;
    }

    struct VulkanDebugMessageContext
    {
        DebugMessageCallback callback;
        void *userData;
    };
} // Hazel
