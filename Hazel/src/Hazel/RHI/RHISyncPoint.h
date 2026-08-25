// Declares the RHI sync point interface.
// Created: 2026-03-16.

#pragma once

#include "RHIBase.h"

#include <cstdint>

namespace Aster
{
    struct RHISyncPoint
    {
        uint64_t value = 0;
        RHIQueue* queue = nullptr;
        bool valid = false;
    };
} // namespace Aster
