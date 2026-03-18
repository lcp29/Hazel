//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include <cstdint>
#include "RHIBase.h"

namespace Hazel
{
    struct RHISyncPoint
    {
        uint64_t value = 0;
        RHIQueue *queue = nullptr;
        bool valid = false;
    };
} // Hazel