//
// Created by helmholtz on 2026/3/14.
//

#pragma once
#include "../RHIAdapter.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    class VulkanAdapter : public RHIAdapter
    {
    public:
        bool CanCreateDevice(const RHIDeviceCapabilities &caps) override;

        VulkanAdapter(vk::PhysicalDevice adapter);

    private:
        vk::PhysicalDevice m_Adapter;
    };
} // Hazel
