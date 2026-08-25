// Implements GPU asset handle resources.
// Created: 2026-04-07.

#include "GPUAssetHandle.h"

#include "GPUAsset.h"
#include "Hazel/Renderer/Renderer.h"

namespace Aster
{
    void GPUAssetHandle::Destroy()
    {
        if (asset && asset->GetUseCount() == 1)
        {
            asset->GetRenderer()->GetGPUAssetRegistry()->RemoveAsset(asset->GetUUID());
            asset = nullptr;
            returnAfterUse = false;
        }
    }

    GPUAssetHandle::~GPUAssetHandle()
    {
        if (asset && returnAfterUse)
        {
            asset->SetLastReferencedFrame(asset->GetRenderer()->GetCurrentFrameIndex());
            asset->Return();
        }
    }
} // namespace Aster
