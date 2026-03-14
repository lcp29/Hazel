//
// Created by helmholtz on 2026/3/14.
//

#include "VulkanAdapter.h"

#include "VulkanCommon.h"

namespace Hazel
{
    bool VulkanAdapter::CanCreateDevice(const RHIDeviceCapabilities &caps)
    {
        return false;
    }

    VulkanAdapter::VulkanAdapter(vk::PhysicalDevice adapter)
    {
        m_Adapter = adapter;

        auto deviceProp = adapter.getProperties();
        m_Info.name = deviceProp.deviceName.data();
        m_Info.deviceId = deviceProp.deviceID;
        m_Info.vendorId = deviceProp.vendorID;
        m_Info.type = VulkanConvertAdapterType(deviceProp.deviceType);
    }
} // Hazel