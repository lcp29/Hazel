//
// Created by helmholtz on 2026/3/23.
//

#pragma once

#include "RenderTextureAsset.h"
#include "SamplerAsset.h"
#include "ComputeShaderAsset.h"
#include "ShaderAsset.h"
#include "TextureAsset.h"
#include "Hazel/Core/UUID.h"

#include <type_traits>
#include <unordered_map>

namespace Hazel
{
    class Project;
    class Renderer;

    enum class AssetType
    {
        Unknown,
        Texture,
        Shader,
        Sampler,
        RenderTexture,
        ComputeShader
    };

    class AssetManager
    {
    public:
        AssetManager() = default;
        AssetManager(Project* project, Renderer* renderer);

        void ScanAll();

        void WriteAllMetaFiles() const;

        template <typename T>
        T* GetAsset(UUID uuid)
        {
            if constexpr (std::is_same_v<T, TextureAsset>)
            {
                auto it = m_Textures.find(uuid);
                return it == m_Textures.end() ? nullptr : &it->second;
            }
            else if constexpr (std::is_same_v<T, ComputeShaderAsset>)
            {
                auto it = m_ComputeShaders.find(uuid);
                return it == m_ComputeShaders.end() ? nullptr : &it->second;
            }
            else if constexpr (std::is_same_v<T, ShaderAsset>)
            {
                auto it = m_Shaders.find(uuid);
                return it == m_Shaders.end() ? nullptr : &it->second;
            }
            else if constexpr (std::is_same_v<T, RenderTextureAsset>)
            {
                auto it = m_RenderTextures.find(uuid);
                return it == m_RenderTextures.end() ? nullptr : &it->second;
            }
            else if constexpr (std::is_same_v<T, SamplerAsset>)
            {
                auto it = m_Samplers.find(uuid);
                return it == m_Samplers.end() ? nullptr : &it->second;
            }

            return nullptr;
        }

        AssetType GetAssetType(UUID uuid) const
        {
            const auto it = m_AssetTypes.find(uuid);
            if (it == m_AssetTypes.end())
            {
                return AssetType::Unknown;
            }
            return it->second;
        }

        const std::unordered_map<UUID, ComputeShaderAsset>& GetComputeShaders() const
        {
            return m_ComputeShaders;
        }

        const std::unordered_map<UUID, ShaderAsset>& GetShaders() const
        {
            return m_Shaders;
        }

        const std::unordered_map<UUID, RenderTextureAsset>& GetRenderTextures() const
        {
            return m_RenderTextures;
        }

        const std::unordered_map<UUID, TextureAsset>& GetTextures() const
        {
            return m_Textures;
        }

        const std::unordered_map<UUID, SamplerAsset>& GetSamplers() const
        {
            return m_Samplers;
        }

    private:
        Project* m_Project = nullptr;
        Renderer* m_Renderer = nullptr;
        std::unordered_map<UUID, AssetType> m_AssetTypes;
        std::unordered_map<UUID, ComputeShaderAsset> m_ComputeShaders;
        std::unordered_map<UUID, ShaderAsset> m_Shaders;
        std::unordered_map<UUID, RenderTextureAsset> m_RenderTextures;
        std::unordered_map<UUID, SamplerAsset> m_Samplers;
        std::unordered_map<UUID, TextureAsset> m_Textures;
    };
} // namespace Hazel
