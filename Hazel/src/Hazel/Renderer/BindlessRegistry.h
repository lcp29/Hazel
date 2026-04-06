//
// Created by helmholtz on 2026/4/6.
//

#pragma once

#include "GPUAsset/GPUAssetResolveResult.h"

#include <vector>

namespace Hazel
{
    constexpr int kBindlessRegistrySize = 65536;

    class Renderer;

    class BindlessRegistry
    {
    public:
        BindlessRegistry() = delete;
        BindlessRegistry(Renderer* renderer);

        uint32_t RegisterTexture(GPUAssetResolveResult textureResult);
        uint32_t RegisterSampler(GPUAssetResolveResult samplerResult);
        uint32_t RegisterSamplerWithImage(GPUAssetResolveResult textureResult,
                                          GPUAssetResolveResult samplerResult);

        void UnregisterTexture(uint32_t index);
        void UnregisterSampler(uint32_t index);
        void UnregisterCombinedImageSampler(uint32_t index);

        const std::vector<GPUAssetResolveResult>& GetTextures() const { return m_Textures; }
        const std::vector<GPUAssetResolveResult>& GetSamplers() const { return m_Samplers; }

        const std::vector<std::pair<GPUAssetResolveResult, GPUAssetResolveResult>>& GetCombinedImageSamplers() const
        {
            return m_CombinedImageSamplers;
        }

    private:
        Renderer* m_Renderer;

        std::mutex m_TextureMutex;
        std::vector<GPUAssetResolveResult> m_Textures;
        std::vector<uint32_t> m_TextureFreeList;
        std::vector<uint8_t> m_TextureFreeMap;

        std::mutex m_SamplerMutex;
        std::vector<GPUAssetResolveResult> m_Samplers;
        std::vector<uint32_t> m_SamplerFreeList;
        std::vector<uint8_t> m_SamplerFreeMap;

        std::mutex m_CombinedImageSamplerMutex;
        std::vector<std::pair<GPUAssetResolveResult, GPUAssetResolveResult>> m_CombinedImageSamplers;
        std::vector<uint32_t> m_CombinedImageSamplerFreeList;
        std::vector<uint8_t> m_CombinedImageSamplerFreeMap;
    };
} // Hazel