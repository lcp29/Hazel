//
// Created by helmholtz on 2026/4/7.
//

#include "GPUAssetHandle.h"
#include "GPUAsset.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel
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
}