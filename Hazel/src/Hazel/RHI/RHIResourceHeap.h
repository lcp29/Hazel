// Declares the RHI resource heap interface.
// Created: 2026-03-15.

#pragma once

#include "RHIBase.h"
#include "RHIResourceGroup.h"
#include "RHIResourceLayout.h"

namespace Aster
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
} // namespace Aster
