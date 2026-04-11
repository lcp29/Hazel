//
// Created by helmholtz on 2026/3/31.
//

#pragma once
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    uint32_t DeduceMipLevelCount(uint32_t width, uint32_t height);
    void ImageUtilGenerateMipmap(RHICommandBuffer* commandBuffer, RHIImage* image);
}