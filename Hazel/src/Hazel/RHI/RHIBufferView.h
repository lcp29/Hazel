//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "RHIImage.h"

#include <cstdint>

namespace Hazel
{
    struct RHIBufferViewDesc
    {
        RHIFormat format = RHIFormat::Undefined;
        uint64_t offset = 0;
        uint64_t range = 0;
    };
} // Hazel
