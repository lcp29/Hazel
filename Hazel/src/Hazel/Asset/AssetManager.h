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

#include <unordered_map>

namespace Hazel
{
    class Project;
    class Renderer;

    class AssetManager
    {
    public:
        AssetManager() = default;
        AssetManager(Project* project, Renderer* renderer);

        void ScanAll();

        void WriteAllMetaFiles() const;

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
        std::unordered_map<UUID, ComputeShaderAsset> m_ComputeShaders;
        std::unordered_map<UUID, ShaderAsset> m_Shaders;
        std::unordered_map<UUID, RenderTextureAsset> m_RenderTextures;
        std::unordered_map<UUID, SamplerAsset> m_Samplers;
        std::unordered_map<UUID, TextureAsset> m_Textures;
    };
} // namespace Hazel
