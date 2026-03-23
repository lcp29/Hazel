//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include "RHIBase.h"
#include "RHIResourceLayout.h"

#include <vector>

namespace Hazel
{
    struct RHIResourceSignatureDesc
    {
        std::vector<RHIResourceLayout*> resourceLayouts;
        std::vector<RHIPushConstantRangeDesc> pushConstantRanges;
    };
} // namespace Hazel