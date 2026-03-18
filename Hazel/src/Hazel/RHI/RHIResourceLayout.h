//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "RHIBase.h"
#include "RHIShader.h"

#include <vector>

namespace Hazel
{
    struct RHIResourceBindingSlotDesc
    {
        uint32_t slot = 0;
        RHIResourceBindingType type = RHIResourceBindingType::UniformBuffer;
        uint32_t count = 1;
        RHIShaderStages stages = {};
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
} // Hazel
