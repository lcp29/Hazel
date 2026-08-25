// Declares the RHI queue interface.
// Created: 2026-03-14.

#pragma once

#include "Flags.h"
#include "RHIBase.h"
#include "RHICommandBuffer.h"
#include "RHISyncPoint.h"

#include <vector>

namespace Aster
{
    struct RHIQueueSubmitDesc
    {
        std::vector<RHICommandBuffer*> commandBuffers;
        std::vector<RHISyncPoint> waitSyncPoints;
    };

    enum class RHIQueueTypeFlagBits : uint8_t
    {
        Graphics = 1 << 0,
        Compute = 1 << 1,
        Transfer = 1 << 2,
        Present = 1 << 3
    };

    using RHIQueueType = RHIQueueTypeFlagBits;

    template <> struct InRHIFlagScope<RHIQueueTypeFlagBits> : std::true_type
    {};

    using RHIQueueTypes = Flags<RHIQueueTypeFlagBits>;
} // namespace Aster
