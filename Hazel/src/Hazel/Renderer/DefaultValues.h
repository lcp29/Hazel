//
// Created by helmholtz on 2026/4/6.
//

#pragma once
#include "Hazel/RHI/RHI.h"
#include "BindlessRegistry.h"

namespace Hazel
{
    inline const RHIResourceLayoutDesc kDefaultResourceLayoutSet0 =
        RHIResourceLayoutDesc{
            .bindings = {
                RHIResourceBindingSlotDesc{
                    .slot = 0,
                    .type = RHIResourceBindingType::UniformDynamicBuffer,
                    .count = 1,
                    .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment
                },
                RHIResourceBindingSlotDesc{
                    .slot = 1,
                    .type = RHIResourceBindingType::SampledImage,
                    .count = kBindlessRegistrySize,
                    .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment
                },
                RHIResourceBindingSlotDesc{
                    .slot = 2,
                    .type = RHIResourceBindingType::Sampler,
                    .count = kBindlessRegistrySize,
                    .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment
                },
                RHIResourceBindingSlotDesc{
                    .slot = 3,
                    .type = RHIResourceBindingType::SamplerWithImage,
                    .count = kBindlessRegistrySize,
                    .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment
                }
            }
        };

    inline const RHIPushConstantRangeDesc kDefaultPushConstantRange =
        RHIPushConstantRangeDesc{
            .offset = 0,
            .size = 128,
            .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment
        };
}