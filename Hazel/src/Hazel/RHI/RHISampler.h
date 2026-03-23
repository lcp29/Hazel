//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include "RHIBase.h"
#include "RHICommon.h"

namespace Hazel
{
    enum class RHISamplerFilter : uint8_t
    {
        Nearest,
        Linear
    };

    enum class RHISamplerAddressMode : uint8_t
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    struct RHISamplerDesc
    {
        RHISamplerFilter minFilter = RHISamplerFilter::Linear;
        RHISamplerFilter magFilter = RHISamplerFilter::Linear;
        RHISamplerFilter mipFilter = RHISamplerFilter::Linear;
        RHISamplerAddressMode addressModeU = RHISamplerAddressMode::Repeat;
        RHISamplerAddressMode addressModeV = RHISamplerAddressMode::Repeat;
        RHISamplerAddressMode addressModeW = RHISamplerAddressMode::Repeat;
        float mipLodBias = 0.0f;
        float minLod = 0.0f;
        float maxLod = 1000.0f;
        float maxAnisotropy = 1.0f;
        bool enableAnisotropy = false;
        bool compareEnable = false;
        RHICompareOp compareOp = RHICompareOp::LessOrEqual;
    };
} // Hazel
