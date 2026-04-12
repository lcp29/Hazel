#include "Hazel/Asset/RenderTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/GPURenderTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/Importer/GPUAssetImporter.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    std::unique_ptr<GPURenderTextureAsset> ImportGPURenderTextureAsset(Renderer* renderer,
                                                                       const RenderTextureAsset* asset)
    {
        return CreateGPURenderTextureAsset(renderer,
                                           asset->GetUUID(),
                                           asset->GetVersion(),
                                           asset->GetMeta().GetDesc(),
                                           renderer->GetCurrentFrameIndex());
    }
} // namespace Hazel