//
// Created by helmholtz on 2026/3/14.
//

#pragma once
#include <vulkan/vulkan.hpp>

#include "../RHIHeaders.h"

namespace Hazel
{
    inline RHIAdapterType VulkanConvertAdapterType(vk::PhysicalDeviceType type)
    {
        switch (type)
        {
            #define X(rhi, vul) case vk::PhysicalDeviceType::vul: return RHIAdapterType::rhi;
            #include "TypeMappings/AdapterTypes.inl"
            #undef X
            default:
                return RHIAdapterType::Other;
        }
    }

    inline vk::Format VulkanConvertFormat(RHIFormat format)
    {
        switch (format)
        {
            #define X(rhi, vul) case RHIFormat::rhi: return vk::Format::vul;
            #include "TypeMappings/Formats.inl"
            #undef X
            default:
                return vk::Format::eUndefined;
        }
    }

    inline RHIFormat VulkanConvertFormat(vk::Format format)
    {
        switch (format)
        {
            #define X(rhi, vul) case vk::Format::vul: return RHIFormat::rhi;
            #include "TypeMappings/Formats.inl"
            #undef X
            default:
                return RHIFormat::Undefined;
        }
    }

    inline vk::ImageUsageFlags VulkanConvertImageUsages(RHIImageUsages usages)
    {
        vk::ImageUsageFlags flags;
        #define X(rhi, vul) if (usages & RHIImageUsageFlagBits::rhi) { flags |= vk::ImageUsageFlagBits::vul; }
        #include "TypeMappings/ImageUsages.inl"
        #undef X
        return flags;
    }

    inline vk::BufferUsageFlags VulkanConvertBufferUsages(RHIBufferUsages usages)
    {
        vk::BufferUsageFlags flags;
        #define X(rhi, vul) if (usages & RHIBufferUsageFlagBits::rhi) { flags |= vk::BufferUsageFlagBits::vul; }
        #include "TypeMappings/BufferUsages.inl"
        #undef X
        return flags;
    }

    inline vk::DescriptorType VulkanConvertResourceBindingType(RHIResourceBindingType type)
    {
        switch (type)
        {
            #define X(rhi, vul) case RHIResourceBindingType::rhi: return vk::DescriptorType::vul;
            #include "TypeMappings/ResourceBindingTypes.inl"
            #undef X
            default:
                return vk::DescriptorType::eUniformBuffer;
        }
    }

    inline vk::ShaderStageFlags VulkanConvertShaderStages(RHIShaderStages stages)
    {
        vk::ShaderStageFlags flags;
        #define X(rhi, vul) if (stages & RHIShaderStageFlagBits::rhi) { flags |= vk::ShaderStageFlagBits::vul; }
        #include "TypeMappings/ShaderStages.inl"
        #undef X
        return flags;
    }

    inline vk::PipelineStageFlags2 VulkanConvertPipelineStages(RHIPipelineStages stages)
    {
        vk::PipelineStageFlags2 flags;
        #define X(rhi, vul) if (stages & RHIPipelineStageFlagBits::rhi) { flags |= vk::PipelineStageFlagBits2::vul; }
        #include "TypeMappings/PipelineStages.inl"
        #undef X
        return flags;
    }

    inline vk::PresentModeKHR VulkanConvertSwapchainMode(RHISwapchainMode mode)
    {
        switch (mode)
        {
            #define X(rhi, vul) case RHISwapchainMode::rhi: return vk::PresentModeKHR::vul;
            #include "TypeMappings/SwapchainModes.inl"
            #undef X
            default:
                return vk::PresentModeKHR::eFifo;
        }
    }

    inline RHISwapchainMode VulkanConvertSwapchainMode(vk::PresentModeKHR mode)
    {
        switch (mode)
        {
            #define X(rhi, vul) case vk::PresentModeKHR::vul: return RHISwapchainMode::rhi;
            #include "TypeMappings/SwapchainModes.inl"
            #undef X
        default:
            return RHISwapchainMode::FIFO;
        }
    }

    inline vk::ImageLayout VulkanConvertImageResourceState(RHIImageResourceState state)
    {
        switch (state)
        {
            #define X(rhi, vul) case RHIImageResourceState::rhi: return vk::ImageLayout::vul;
            #include "TypeMappings/ResourceStates.inl"
            #undef X
            default:
                return vk::ImageLayout::eUndefined;
        }
    }

    inline vk::ComponentSwizzle VulkanConvertImageViewComponent(RHIImageViewComponent component)
    {
        switch (component)
        {
            #define X(rhi, vul) case RHIImageViewComponent::rhi: return vk::ComponentSwizzle::vul;
            #include "TypeMappings/ImageViewComponents.inl"
            #undef X
            default:
                return vk::ComponentSwizzle::eIdentity;
        }
    }

    inline vk::ImageViewType VulkanConvertImageViewType(RHIImageViewType type)
    {
        switch (type)
        {
            #define X(rhi, vul) case RHIImageViewType::rhi: return vk::ImageViewType::vul;
            #include "TypeMappings/ImageViewTypes.inl"
            #undef X
            default:
                return vk::ImageViewType::e2D;
        }
    }

    inline vk::ComponentMapping VulkanConvertImageViewComponentMapping(const RHIImageViewComponentMapping &mapping)
    {
        return {
            VulkanConvertImageViewComponent(mapping.r),
            VulkanConvertImageViewComponent(mapping.g),
            VulkanConvertImageViewComponent(mapping.b),
            VulkanConvertImageViewComponent(mapping.a)
        };
    }

    inline VkDebugUtilsMessageSeverityFlagsEXT VulkanConvertDebugMessageSeverity(DebugMessageSeverity severity)
    {
        VkDebugUtilsMessageSeverityFlagsEXT flags = 0;
        #define X(rhi, vul) if (severity & DebugMessageSeverityFlagBits::rhi) { flags |= vul; }
        #include "TypeMappings/DebugMessageSeverities.inl"
        #undef X
        return flags;
    }

    inline VkDebugUtilsMessageTypeFlagsEXT VulkanConvertDebugMessageType(DebugMessageType type)
    {
        VkDebugUtilsMessageTypeFlagsEXT flags = 0;
        #define X(rhi, vul) if (type & DebugMessageTypeFlagBits::rhi) { flags |= vul; }
        #include "TypeMappings/DebugMessageTypes.inl"
        #undef X
        return flags;
    }

    inline vk::ImageAspectFlags VulkanConvertImagePlanes(RHIImagePlanes flags)
    {
        vk::ImageAspectFlags result;
        #define X(rhi, vul) if (flags & RHIImagePlaneFlagBits::rhi) { result |= vk::ImageAspectFlagBits::vul; }
        #include "TypeMappings/ImagePlanes.inl"
        #undef X
        return result;
    }

    inline vk::AccessFlags2 VulkanConvertAccessFlags(RHIPipelineAccessFlags flags)
    {
        vk::AccessFlags2 result;
        #define X(rhi, vul) if (flags & RHIPipelineAccessFlagBits::rhi) { result |= vk::AccessFlagBits2::vul; }
        #include "TypeMappings/AccessFlags.inl"
        #undef X
        return result;
    }

    struct VulkanDebugMessageContext
    {
        DebugMessageCallback callback;
        void *userData;
    };
} // Hazel
