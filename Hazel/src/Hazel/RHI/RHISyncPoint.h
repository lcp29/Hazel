//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include "RHIBase.h"

#include <cstdint>

namespace Hazel
{
    struct RHISyncPoint
    {
        uint64_t value = 0;
        RHIQueue* queue = nullptr;
        bool valid = false;
    };
} // namespace Hazel