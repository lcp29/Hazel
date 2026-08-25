// Declares the RHI command pool interface.
// Created: 2026-03-14.

#pragma once

#include "RHIQueue.h"

namespace Aster
{
    struct RHICommandPoolDesc
    {
        RHIQueueType queueType = RHIQueueType::Graphics;
        bool transient = false;
        bool allowCommandBufferReset = false;
    };
} // namespace Aster
