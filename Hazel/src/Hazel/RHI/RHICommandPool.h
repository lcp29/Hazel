//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "RHIQueue.h"

namespace Hazel
{
    struct RHICommandPoolDesc
    {
        RHIQueueType queueType = RHIQueueType::Graphics;
        bool transient = false;
        bool allowCommandBufferReset = false;
    };
} // namespace Hazel
