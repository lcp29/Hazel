//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "Flags.h"

namespace Hazel
{
    enum class RHIFormat
    {
        Undefined,
        R32SInt,
        R32SFloat,
        RG32SFloat,
        RGB32SFloat,
        RG16UNorm,
        BGRA8UNorm,
        BGRA8SRGB,
        RGBA8UNorm,
        RGBA8SRGB,
        RGB10A2UNorm,
        RGBA16SFloat,
        D32SFloat,
        D32SFloatS8Uint,
        S8Uint
    };

    enum class RHIImagePlaneFlagBits : uint16_t
    {
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2
    };

    template<>
    struct InRHIFlagScope<RHIImagePlaneFlagBits> : std::true_type {};

    using RHIImagePlanes = Flags<RHIImagePlaneFlagBits>;

    enum class RHIImageUsageFlagBits : uint16_t
    {
        TransferSource = 1 << 0,
        TransferDestination = 1 << 1,
        Sampled = 1 << 2,
        Storage = 1 << 3,
        ColorAttachment = 1 << 4,
        DepthStencilAttachment = 1 << 5
    };

    template<>
    struct InRHIFlagScope<RHIImageUsageFlagBits> : std::true_type {};

    using RHIImageUsages = Flags<RHIImageUsageFlagBits>;

    enum class RHIImageResourceState
    {
        Undefined,
        Common,
        TransferSource,
        TransferDestination,
        ShaderRead,
        ShaderWrite,
        ColorAttachment,
        DepthAttachment,
        StencilAttachment,
        DepthStencilAttachment,
        Present
    };

    struct RHIImageDesc
    {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
        RHIFormat format = RHIFormat::Undefined;
        RHIImageUsages usages = {};
        RHIImageResourceState initialState = RHIImageResourceState::Undefined;
    };

    struct RHIImageSubresourceRange
    {
        uint32_t baseMipLevel = 0;
        uint32_t levelCount = 1;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
        RHIImagePlanes planes = RHIImagePlaneFlagBits::Color;
    };
} // Hazel
