//
// Created by helmholtz on 2026/3/23.
//

#pragma once

#include "RenderTextureAsset.h"
#include "SamplerAsset.h"
#include "ComputeShaderAsset.h"
#include "ShaderAsset.h"
#include "TextureAsset.h"
#include "MaterialAsset.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/Mesh.h"

#include <filesystem>
#include <fstream>
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
        ComputeShader,
        Mesh,
        Material
    };

    class AssetManager
    {
    public:
        AssetManager() = default;

        AssetManager(Project* project, Renderer* renderer);

        AssetManager(AssetManager&& other) noexcept;
        AssetManager& operator=(AssetManager&& other) noexcept;

        ~AssetManager()
        {
            UnloadAllAssets();
        }

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
            else if constexpr (std::is_same_v<T, MeshAsset>)
            {
                auto it = m_Meshes.find(uuid);
                return it == m_Meshes.end() ? nullptr : &it->second;
            }
            else if constexpr (std::is_same_v<T, MaterialAsset>)
            {
                auto it = m_Materials.find(uuid);
                return it == m_Materials.end() ? nullptr : &it->second;
            }

            return nullptr;
        }

        template <typename T, typename... Args>
        T* AddAsset(UUID uuid, Args&&... args)
        {
            if constexpr (std::is_same_v<T, ComputeShaderAsset>)
            {
                auto [it, inserted] = m_ComputeShaders.emplace(uuid, T(std::forward<Args>(args)...));
                m_AssetTypes[uuid] = AssetType::ComputeShader;
                return &it->second;
            }
            else if constexpr (std::is_same_v<T, ShaderAsset>)
            {
                auto [it, inserted] = m_Shaders.emplace(uuid, T(std::forward<Args>(args)...));
                m_AssetTypes[uuid] = AssetType::Shader;
                return &it->second;
            }
            else if constexpr (std::is_same_v<T, RenderTextureAsset>)
            {
                auto [it, inserted] = m_RenderTextures.emplace(uuid, T(std::forward<Args>(args)...));
                m_AssetTypes[uuid] = AssetType::RenderTexture;
                return &it->second;
            }
            else if constexpr (std::is_same_v<T, SamplerAsset>)
            {
                auto [it, inserted] = m_Samplers.emplace(uuid, T(std::forward<Args>(args)...));
                m_AssetTypes[uuid] = AssetType::Sampler;
                return &it->second;
            }
            else if constexpr (std::is_same_v<T, MaterialAsset>)
            {
                auto [it, inserted] = m_Materials.emplace(uuid, T(std::forward<Args>(args)...));
                m_AssetTypes[uuid] = AssetType::Material;
                return &it->second;
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

        std::unordered_map<UUID, ComputeShaderAsset>& GetComputeShaders()
        {
            return m_ComputeShaders;
        }

        std::unordered_map<UUID, ShaderAsset>& GetShaders()
        {
            return m_Shaders;
        }

        std::unordered_map<UUID, RenderTextureAsset>& GetRenderTextures()
        {
            return m_RenderTextures;
        }

        std::unordered_map<UUID, TextureAsset>& GetTextures()
        {
            return m_Textures;
        }

        std::unordered_map<UUID, SamplerAsset>& GetSamplers()
        {
            return m_Samplers;
        }

        std::unordered_map<UUID, MeshAsset>& GetMeshes()
        {
            return m_Meshes;
        }

        std::unordered_map<UUID, MaterialAsset>& GetMaterials()
        {
            return m_Materials;
        }

        void UnloadAllAssets();
        void LoadAssetUUIDs(const std::vector<UUID>& uuids);

    private:
        Project* m_Project = nullptr;
        Renderer* m_Renderer = nullptr;
        std::unordered_map<UUID, AssetType> m_AssetTypes;
        std::unordered_map<UUID, ComputeShaderAsset> m_ComputeShaders;
        std::unordered_map<UUID, ShaderAsset> m_Shaders;
        std::unordered_map<UUID, RenderTextureAsset> m_RenderTextures;
        std::unordered_map<UUID, SamplerAsset> m_Samplers;
        std::unordered_map<UUID, TextureAsset> m_Textures;
        std::unordered_map<UUID, MeshAsset> m_Meshes;
        std::unordered_map<UUID, MaterialAsset> m_Materials;
    };
} // namespace Hazel