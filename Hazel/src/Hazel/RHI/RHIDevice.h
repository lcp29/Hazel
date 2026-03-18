//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "RHICommon.h"
#include "RHIQueue.h"

namespace Hazel
{
    struct RHIDeviceCapabilities
    {
        RHIQueueTypes queueTypes = {};

        bool supportSubgroup = false;
        uint32_t subgroupSizeMin = 0;
        uint32_t subgroupSizeMax = 0;
    };
} // namespace Hazel
