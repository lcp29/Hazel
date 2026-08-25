// Declares GPU asset handle resources.
// Created: 2026-04-05.

#pragma once
#include "GPUAsset.h"

namespace Aster
{
    struct GPUAssetHandle
    {
        GPUAsset* asset = nullptr;
        bool returnAfterUse = true;

        GPUAssetHandle(const GPUAssetHandle&) = delete;
        GPUAssetHandle& operator=(const GPUAssetHandle&) = delete;

        GPUAssetHandle(GPUAsset* asset)
            : asset(asset)
        {}

        GPUAssetHandle(GPUAsset* asset, bool returnAfterUse)
            : asset(asset)
            , returnAfterUse(returnAfterUse)
        {}

        GPUAssetHandle(GPUAssetHandle&& other) noexcept
            : asset(other.asset)
            , returnAfterUse(other.returnAfterUse)
        {
            other.asset = nullptr;
            other.returnAfterUse = false;
        }

        GPUAssetHandle& operator=(GPUAssetHandle&& other) noexcept
        {
            if (this == &other) { return *this; }
            asset = other.asset;
            returnAfterUse = other.returnAfterUse;
            other.asset = nullptr;
            other.returnAfterUse = false;
            return *this;
        }

        void Destroy();

        ~GPUAssetHandle();
    };
} // namespace Aster
