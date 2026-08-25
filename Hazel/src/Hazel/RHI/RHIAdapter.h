// Declares the RHI adapter interface.
// Created: 2026-03-14.

#pragma once

#include "RHIBase.h"
#include "RHICommon.h"

#include <string>

namespace Aster
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
        uint32_t deviceId = 0;
        uint32_t vendorId = 0;
        RHIAdapterType type = RHIAdapterType::Other;
    };
} // namespace Aster
