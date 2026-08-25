// Declares the RHI image view interface.
// Created: 2026-03-15.

#pragma once

#include "RHICommon.h"
#include "RHIImage.h"

namespace Aster
{
    enum RHIImageViewComponent
    {
        Identity,
        Zero,
        One,
        R,
        G,
        B,
        A
    };

    enum RHIImageViewType
    {
        Image1D,
        Image2D,
        Image3D,
        Cube,
        Image1DArray,
        Image2DArray,
        CubeArray
    };

    struct RHIImageViewComponentMapping
    {
        RHIImageViewComponent r = Identity;
        RHIImageViewComponent g = Identity;
        RHIImageViewComponent b = Identity;
        RHIImageViewComponent a = Identity;
    };

    struct RHIImageViewDesc
    {
        RHIFormat format = RHIFormat::Undefined;
        RHIImageViewType viewType = Image2D;
        RHIImageViewComponentMapping componentMapping;
        RHIImageSubresourceRange subresourceRange;
    };
} // namespace Aster
