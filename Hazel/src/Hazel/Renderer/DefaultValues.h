// Declares default renderer resource layouts and push constants.
// Created: 2026-04-06.

#pragma once
#include "BindlessRegistry.h"
#include "Hazel/RHI/RHI.h"

namespace Aster
{
    inline const RHIResourceLayoutDesc kDefaultResourceLayoutSet0 = RHIResourceLayoutDesc{
        .bindings = {
            RHIResourceBindingSlotDesc{.slot = 0,
                                       .type = RHIResourceBindingType::UniformDynamicBuffer,
                                       .count = 1,
                                       .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment},
            RHIResourceBindingSlotDesc{.slot = 1,
                                       .type = RHIResourceBindingType::SampledImage,
                                       .count = kBindlessRegistrySize,
                                       .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment},
            RHIResourceBindingSlotDesc{.slot = 2,
                                       .type = RHIResourceBindingType::Sampler,
                                       .count = kBindlessRegistrySize,
                                       .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment},
            RHIResourceBindingSlotDesc{.slot = 3,
                                       .type = RHIResourceBindingType::SamplerWithImage,
                                       .count = kBindlessRegistrySize,
                                       .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment}}};

    inline const RHIPushConstantRangeDesc kDefaultPushConstantRange = RHIPushConstantRangeDesc{
        .offset = 0, .size = 128, .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment};
} // namespace Aster
