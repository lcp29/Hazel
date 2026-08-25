// Declares asset discovery, loading, and dependency management.
// Created: 2026-03-23.

#pragma once

#include "../Renderer/GPUAsset/GPUMeshAsset.h"
#include "ComputeShaderAsset.h"
#include "Hazel/Core/UUID.h"
#include "MaterialAsset.h"
#include "RenderTextureAsset.h"
#include "SamplerAsset.h"
#include "ShaderAsset.h"
#include "TextureAsset.h"

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
} // namespace Hazel

namespace Aster
{

    class AssetManager
    {
      public:
        AssetManager() = delete;

        AssetManager(Hazel::Project* project, Hazel::Renderer* renderer);

        void WriteAllMetaFiles() const;
        void InitializeAssetRegistry();
        AssetType GetAssetType(Hazel::UUID uuid) const;
        bool HasAsset(Hazel::UUID uuid) const;
        std::filesystem::path GetAssetPath(Hazel::UUID uuid) const;
        std::vector<AssetRegistryTerm*> GetAssetsByType(AssetType type) const;
        void RegisterAsset(std::unique_ptr<AssetRegistryTerm> term);
        void ClearLoadedAssets();
        Asset* RequestAsset(Hazel::UUID uuid);
        Asset* RequestAssetBlocked(Hazel::UUID uuid);

      private:
        Asset* LoadAssetFromRegistry(Hazel::UUID uuid, AssetRegistryTerm* registry);

        Hazel::Project* m_Project = nullptr;
        Hazel::Renderer* m_Renderer = nullptr;

        mutable std::mutex m_AssetRegistryMutex;
        std::unordered_map<Hazel::UUID, std::unique_ptr<AssetRegistryTerm>> m_AssetRegistry;
        mutable std::mutex m_AssetMutex;
        std::unordered_map<Hazel::UUID, std::unique_ptr<Asset>> m_Assets;

        mutable std::mutex m_DependencyMutex;
        std::unordered_map<Hazel::UUID, std::unordered_set<Hazel::UUID>> m_Dependencies;
        std::unordered_map<Hazel::UUID, std::unordered_set<Hazel::UUID>> m_Dependents;
    };
} // namespace Aster
