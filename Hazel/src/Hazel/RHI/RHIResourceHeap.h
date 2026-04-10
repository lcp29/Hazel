//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "RHIBase.h"
#include "RHIResourceGroup.h"
#include "RHIResourceLayout.h"

namespace Hazel
{
    struct RHIResourceHeapDesc
    {
        uint32_t maxGroups = 1;
        uint32_t samplerCount = 0;
        uint32_t samplerWithImageCount = 0;
        uint32_t sampledImageCount = 0;
        uint32_t storageImageCount = 0;
        uint32_t uniformBufferCount = 0;
        uint32_t storageBufferCount = 0;
        uint32_t uniformTexelBufferCount = 0;
        uint32_t storageTexelBufferCount = 0;
        bool updateAfterBind = false;
    };
} // namespace Hazel