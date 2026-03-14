//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include <string>

#include "RHICommon.h"

namespace Hazel
{
    enum class RHIAdapterType
    {
        CPU,
        IntegratedGPU,
        DiscreteGPU
    };

    struct RHIAdapterInfo
    {
        std::string name;
        uint32_t deviceId;
        uint32_t vendorId;
        RHIAdapterType type;
    };

    class RHIAdapter
    {
    public:
        virtual bool CanCreateDevice(const RHIDeviceCapabilities &caps) = 0;

        std::string GetName() { return m_Info.name; }
        uint32_t GetDeviceId() const { return m_Info.deviceId; }
        uint32_t GetVendorId() const { return m_Info.vendorId; }
        RHIAdapterType GetType() const { return m_Info.type; }

        RHIAdapter() = default;

        virtual ~RHIAdapter() = default;

    protected:
        RHIAdapterInfo m_Info;
    };
} // Hazel
