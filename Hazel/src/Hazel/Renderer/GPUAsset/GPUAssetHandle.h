//
// Created by helmholtz on 2026/4/5.
//

#pragma once
#include "GPUAsset.h"

namespace Hazel
{
    struct GPUAssetHandle
    {
        GPUAsset* asset = nullptr;
        bool returnAfterUse = true;

        GPUAssetHandle(const GPUAssetHandle&) = delete;
        GPUAssetHandle& operator=(const GPUAssetHandle&) = delete;

        GPUAssetHandle(GPUAsset* asset)
            : asset(asset) {}

        GPUAssetHandle(GPUAsset* asset, bool returnAfterUse)
            : asset(asset), returnAfterUse(returnAfterUse) {}

        GPUAssetHandle(GPUAssetHandle&& other) noexcept
            : asset(other.asset), returnAfterUse(other.returnAfterUse)
        {
            other.asset = nullptr;
            other.returnAfterUse = false;
        }

        GPUAssetHandle& operator=(GPUAssetHandle&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }
            asset = other.asset;
            returnAfterUse = other.returnAfterUse;
            other.asset = nullptr;
            other.returnAfterUse = false;
            return *this;
        }

        void Destroy();

        ~GPUAssetHandle();
    };
}