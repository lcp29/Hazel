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
#include "../Renderer/GPUAsset/GPUMeshAsset.h"

#include <FileWatch.hpp>

#include <filesystem>
#include <fstream>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Hazel
{
    class Project;
    class Renderer;

    class AssetManager
    {
    public:
        AssetManager() = delete;

        AssetManager(Project* project, Renderer* renderer);

        void WriteAllMetaFiles() const;
        void InitializeAssetRegistry();
        AssetType GetAssetType(UUID uuid) const;
        bool HasAsset(UUID uuid) const;
        std::filesystem::path GetAssetPath(UUID uuid) const;
        std::vector<AssetRegistryTerm*> GetAssetsByType(AssetType type) const;
        void RegisterAsset(std::unique_ptr<AssetRegistryTerm> term);
        void ClearLoadedAssets();
        Asset* RequestAsset(UUID uuid);
        Asset* RequestAssetBlocked(UUID uuid);

    private:
        Asset* LoadAssetFromRegistry(UUID uuid, AssetRegistryTerm* registry);

        Project* m_Project = nullptr;
        Renderer* m_Renderer = nullptr;

        mutable std::mutex m_AssetRegistryMutex;
        std::unordered_map<UUID, std::unique_ptr<AssetRegistryTerm>> m_AssetRegistry;
        mutable std::mutex m_AssetMutex;
        std::unordered_map<UUID, std::unique_ptr<Asset>> m_Assets;

        mutable std::mutex m_DependencyMutex;
        std::unordered_map<UUID, std::unordered_set<UUID>> m_Dependencies;
        std::unordered_map<UUID, std::unordered_set<UUID>> m_Dependents;
    };
} // namespace Hazel