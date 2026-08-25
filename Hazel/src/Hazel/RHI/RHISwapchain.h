// Declares the RHI swapchain interface.
// Created: 2026-03-14.

#pragma once

#include "RHIBase.h"
#include "RHICommon.h"
#include "RHIImage.h"
#include "RHISurface.h"
#include "RHISyncPoint.h"

namespace Aster
{
    enum class RHISwapchainMode
    {
        Immediate,
        Mailbox,
        FIFO,
        FIFORelaxed
    };

    struct RHISwapchainDesc
    {
        RHISurface* surface = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t imageCount = 2;
        RHIFormat format = RHIFormat::BGRA8UNorm;
        RHISwapchainMode mode = RHISwapchainMode::FIFO;
        RHIImageUsages usages = RHIImageUsageFlagBits::ColorAttachment;
    };

    struct RHISwapchainAcquireResult
    {
        uint32_t frameNumber = 0;
        RHISyncPoint availableSyncPoint;
    };
} // namespace Aster
