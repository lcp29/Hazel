// Declares image loading and format utilities.
// Created: 2026-03-31.

#pragma once
#include "Hazel/RHI/RHI.h"

namespace Aster
{
    uint32_t DeduceMipLevelCount(uint32_t width, uint32_t height);
    void ImageUtilGenerateMipmap(RHICommandBuffer* commandBuffer, RHIImage* image);
} // namespace Aster
