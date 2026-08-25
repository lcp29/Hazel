// Declares the RHI surface interface.
// Created: 2026-03-14.

#pragma once

#include "RHIBase.h"
#include "RHICommon.h"

namespace Aster
{
    struct RHISurfaceDesc
    {
        void* backendHandle = nullptr;
    };
} // namespace Aster
