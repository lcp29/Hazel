// Declares the RHI buffer view interface.
// Created: 2026-03-15.

#pragma once

#include "RHIImage.h"

#include <cstdint>

namespace Aster
{
    struct RHIBufferViewDesc
    {
        RHIFormat format = RHIFormat::Undefined;
        uint64_t offset = 0;
        uint64_t range = 0;
    };
} // namespace Aster
