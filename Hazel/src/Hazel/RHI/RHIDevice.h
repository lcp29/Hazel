// Declares the RHI device interface.
// Created: 2026-03-14.

#pragma once

#include "RHICommon.h"
#include "RHIQueue.h"

namespace Aster
{
    struct RHIDeviceCapabilities
    {
        RHIQueueTypes queueTypes = {};

        bool supportSubgroup = false;
        uint32_t subgroupSizeMin = 0;
        uint32_t subgroupSizeMax = 0;
    };
} // namespace Aster
