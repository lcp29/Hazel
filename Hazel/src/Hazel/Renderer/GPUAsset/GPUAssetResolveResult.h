//
// Created by helmholtz on 2026/4/5.
//

#pragma once
#include "GPUAsset.h"

namespace Hazel
{
    struct GPUAssetResolveResult
    {
        GPUAsset* asset = nullptr;
        bool returnAfterUse = true;

        GPUAssetResolveResult(const GPUAssetResolveResult&) = delete;
        GPUAssetResolveResult& operator=(const GPUAssetResolveResult&) = delete;

        GPUAssetResolveResult(GPUAsset* asset)
            : asset(asset) {}

        GPUAssetResolveResult(GPUAsset* asset, bool returnAfterUse)
            : asset(asset), returnAfterUse(returnAfterUse) {}

        GPUAssetResolveResult(GPUAssetResolveResult&& other) noexcept
            : asset(other.asset), returnAfterUse(other.returnAfterUse)
        {
            other.asset = nullptr;
            other.returnAfterUse = false;
        }

        GPUAssetResolveResult& operator=(GPUAssetResolveResult&& other) noexcept
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

        ~GPUAssetResolveResult();
    };
}