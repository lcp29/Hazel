// Declares the RHI resource layout interface.
// Created: 2026-03-15.

#pragma once

#include "RHIBase.h"
#include "RHIShader.h"

#include <vector>

namespace Aster
{
    struct RHIResourceBindingSlotDesc
    {
        uint32_t slot = 0;
        RHIResourceBindingType type = RHIResourceBindingType::UniformBuffer;
        uint32_t count = 1;
        RHIShaderStages stages = {};
        bool updateAfterBind = false;
        bool partiallyBound = false;
    };

    struct RHIPushConstantRangeDesc
    {
        uint32_t offset = 0;
        uint32_t size = 0;
        RHIShaderStages stages = {};
    };

    struct RHIResourceLayoutDesc
    {
        std::vector<RHIResourceBindingSlotDesc> bindings;
    };
} // namespace Aster
