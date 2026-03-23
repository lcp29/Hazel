//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "RHIBase.h"
#include "RHICommon.h"

#include <string>

namespace Hazel
{
    enum class RHIAdapterType
    {
        CPU,
        IntegratedGPU,
        DiscreteGPU,
        Other
    };

    struct RHIAdapterInfo
    {
        std::string name;
        uint32_t deviceId;
        uint32_t vendorId;
        RHIAdapterType type;
    };
} // namespace Hazel