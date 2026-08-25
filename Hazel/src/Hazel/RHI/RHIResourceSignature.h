// Declares the RHI resource signature interface.
// Created: 2026-03-16.

#pragma once

#include "RHIBase.h"
#include "RHIResourceLayout.h"

#include <vector>

namespace Aster
{
    struct RHIResourceSignatureDesc
    {
        std::vector<RHIResourceLayout*> resourceLayouts;
        std::vector<RHIPushConstantRangeDesc> pushConstantRanges;
    };
} // namespace Aster
