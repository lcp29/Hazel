#include "Hazel/Renderer/GPUAsset/Importer/GPUAssetImporter.h"

#include "Hazel/Asset/SamplerAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUSamplerAsset.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    std::unique_ptr<GPUSamplerAsset> ImportGPUSamplerAsset(Renderer* renderer, const SamplerAsset* asset)
    {
        const auto& meta = asset->GetMeta();
        const auto& samplerDesc = meta.GetDesc();
        auto* sampler = renderer->GetDevice()->CreateSampler(samplerDesc);

        return std::make_unique<GPUSamplerAsset>(asset->GetUUID(),
                                                 asset->GetVersion(),
                                                 renderer,
                                                 samplerDesc,
                                                 sampler,
                                                 renderer->GetCurrentFrameIndex());
    }
} // namespace Hazel
