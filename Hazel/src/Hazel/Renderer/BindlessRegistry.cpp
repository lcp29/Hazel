//
// Created by helmholtz on 2026/4/6.
//

#include "BindlessRegistry.h"

namespace Hazel
{
    BindlessRegistry::BindlessRegistry(Renderer* renderer)
        : m_Renderer(renderer)
    {
        m_TextureFreeMap.resize(kBindlessRegistrySize, true);
        m_SamplerFreeMap.resize(kBindlessRegistrySize, true);
        m_CombinedImageSamplerFreeMap.resize(kBindlessRegistrySize, true);
    }

    uint32_t BindlessRegistry::RegisterTexture(GPUAssetResolveResult textureResult)
    {
        if (!textureResult.asset)
        {
            return -1;
        }

        std::unique_lock lock(m_TextureMutex);
        if (!m_TextureFreeList.empty())
        {
            const uint32_t index = m_TextureFreeList.back();
            m_TextureFreeList.pop_back();
            m_TextureFreeMap[index] = false;
            m_Textures[index] = std::move(textureResult);
            return index;
        }

        m_Textures.push_back(std::move(textureResult));
        m_TextureFreeMap[m_Textures.size() - 1] = false;
        return m_Textures.size() - 1;
    }

    uint32_t BindlessRegistry::RegisterSampler(GPUAssetResolveResult samplerResult)
    {
        if (!samplerResult.asset)
        {
            return -1;
        }

        std::unique_lock lock(m_SamplerMutex);
        if (!m_SamplerFreeList.empty())
        {
            const uint32_t index = m_SamplerFreeList.back();
            m_SamplerFreeList.pop_back();
            m_SamplerFreeMap[index] = false;
            m_Samplers[index] = std::move(samplerResult);
            return index;
        }

        m_Samplers.push_back(std::move(samplerResult));
        m_SamplerFreeMap[m_Samplers.size() - 1] = false;
        return m_Samplers.size() - 1;
    }

    uint32_t BindlessRegistry::RegisterSamplerWithImage(GPUAssetResolveResult textureResult,
                                                        GPUAssetResolveResult samplerResult)
    {
        if (!textureResult.asset || !samplerResult.asset)
        {
            return -1;
        }

        std::unique_lock lock(m_CombinedImageSamplerMutex);
        if (!m_CombinedImageSamplerFreeList.empty())
        {
            const uint32_t index = m_CombinedImageSamplerFreeList.back();
            m_CombinedImageSamplerFreeList.pop_back();
            m_CombinedImageSamplerFreeMap[index] = false;
            m_CombinedImageSamplers[index] = std::move(std::make_pair(std::move(textureResult),
                                                                      std::move(samplerResult)));
            return index;
        }

        m_CombinedImageSamplers.emplace_back(std::move(textureResult), std::move(samplerResult));
        m_CombinedImageSamplerFreeMap[m_CombinedImageSamplers.size() - 1] = false;
        return m_CombinedImageSamplers.size() - 1;
    }

    void BindlessRegistry::UnregisterTexture(uint32_t index)
    {
        if (index >= m_Textures.size() || m_TextureFreeMap[index])
        {
            return;
        }
        m_Textures[index] = {nullptr, false};
        m_TextureFreeMap[index] = true;
        m_TextureFreeList.push_back(index);
    }

    void BindlessRegistry::UnregisterSampler(uint32_t index)
    {
        if (index >= m_Samplers.size() || m_SamplerFreeMap[index])
        {
            return;
        }
        m_Samplers[index] = {nullptr, false};
        m_SamplerFreeMap[index] = true;
        m_SamplerFreeList.push_back(index);
    }

    void BindlessRegistry::UnregisterCombinedImageSampler(uint32_t index)
    {
        if (index >= m_CombinedImageSamplers.size() || m_CombinedImageSamplerFreeMap[index])
        {
            return;
        }
        auto combinedResult = std::move(m_CombinedImageSamplers[index]);
        m_CombinedImageSamplers[index] = {nullptr, nullptr};
        m_CombinedImageSamplerFreeMap[index] = true;
        m_CombinedImageSamplerFreeList.push_back(index);
    }
} // Hazel