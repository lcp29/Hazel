//
// Created by helmholtz on 2026/4/7.
//

#include "GPUAssetResolveResult.h"
#include "GPUAsset.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    GPUAssetResolveResult::~GPUAssetResolveResult() {
        if (asset && returnAfterUse)
        {
            asset->SetLastReferencedFrame(asset->GetRenderer()->GetCurrentFrameIndex());
            asset->Return();
        }
    }
}