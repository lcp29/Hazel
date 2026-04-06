//
// Created by helmholtz on 2026/3/23.
//

#include "AssetManager.h"

#include "AssetImporter.h"
#include "AssetUtils.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Renderer/Renderer.h"

#include <algorithm>
#include <ranges>
#include <thread>

namespace Hazel
{
    AssetManager::AssetManager(Project* project, Renderer* renderer)
        : m_Project(project), m_Renderer(renderer) {}

    void AssetManager::WriteAllMetaFiles() const
    {
        std::unique_lock assetLock(m_AssetMutex);
        for (auto& asset : m_Assets | std::views::values)
        {
            auto type = asset->GetType();
            switch (type)
            {
                case AssetType::Texture:
                    WriteMetaToFile(static_cast<TextureAsset*>(asset.get())->GetMeta(),
                                    asset->GetFilePath().string() + ".meta");
                    break;
                case AssetType::Shader:
                    WriteMetaToFile(static_cast<ShaderAsset*>(asset.get())->GetMeta(),
                                    asset->GetFilePath().string() + ".meta");
                    break;
                case AssetType::Sampler:
                    WriteMetaToFile(static_cast<SamplerAsset*>(asset.get())->GetMeta(),
                                    asset->GetFilePath().string());
                    break;
                case AssetType::RenderTexture:
                    WriteMetaToFile(static_cast<RenderTextureAsset*>(asset.get())->GetMeta(),
                                    asset->GetFilePath().string());
                    break;
                case AssetType::ComputeShader:
                    WriteMetaToFile(static_cast<ComputeShaderAsset*>(asset.get())->GetMeta(),
                                    asset->GetFilePath().string() + ".meta");
                    break;
                case AssetType::Mesh:
                    WriteMetaToFile(static_cast<MeshAsset*>(asset.get())->GetMeta(),
                                    asset->GetFilePath().string() + ".meta");
                    break;
                case AssetType::Material:
                    WriteMetaToFile(static_cast<MaterialAsset*>(asset.get())->GetMeta(),
                                    asset->GetFilePath().string());
                    break;
                default:
                    break;
            }
        }
    }

    void AssetManager::InitializeAssetRegistry()
    {
        auto assetDirectory = m_Project->GetAssetDirectory();

        for (const auto& file : std::filesystem::recursive_directory_iterator(assetDirectory))
        {
            if (file.is_regular_file())
            {
                const auto& assetPath = file.path();

                auto assetType = InferAssetTypeFromPath(assetPath);

                if (assetType == AssetType::Unknown)
                {
                    continue;
                }

                auto metaPath = GetMetaPathFromAssetPath(assetPath);

                if (!std::filesystem::exists(metaPath))
                {
                    switch (assetType)
                    {
                        case AssetType::Texture:
                            WriteMetaToFile(TextureAssetMeta::CreateDefault(), metaPath);
                            break;
                        case AssetType::Shader:
                            WriteMetaToFile(ShaderAssetMeta::CreateDefault(), metaPath);
                            break;
                        case AssetType::ComputeShader:
                            WriteMetaToFile(ComputeShaderAssetMeta::CreateDefault(), metaPath);
                            break;
                        case AssetType::Mesh:
                            WriteMetaToFile(MeshAssetMeta::CreateDefault(), metaPath);
                            break;
                        default:
                            continue;
                    }
                }

                std::unique_ptr<AssetRegistryTerm> registryTerm = std::make_unique<AssetRegistryTerm>();
                registryTerm->type = assetType;
                registryTerm->filePath = assetPath;
                registryTerm->state = AssetState::Unloaded;

                auto uuid = GetUUIDFromMetaFile(metaPath);
                if (uuid != UUID(-1))
                {
                    registryTerm->uuid = uuid;
                    std::unique_lock registryLock(m_AssetRegistryMutex);
                    m_AssetRegistry.emplace(uuid, std::move(registryTerm));
                    continue;
                }

                switch (assetType)
                {
                    case AssetType::Texture:
                    {
                        TextureAssetMeta meta = TextureAssetMeta::CreateDefault();
                        WriteMetaToFile(meta, metaPath);
                        registryTerm->uuid = meta.GetUUID();
                        std::unique_lock registryLock(m_AssetRegistryMutex);
                        m_AssetRegistry.emplace(meta.GetUUID(), std::move(registryTerm));
                        break;
                    }
                    case AssetType::Shader:
                    {
                        ShaderAssetMeta meta = ShaderAssetMeta::CreateDefault();
                        WriteMetaToFile(meta, metaPath);
                        registryTerm->uuid = meta.GetUUID();
                        std::unique_lock registryLock(m_AssetRegistryMutex);
                        m_AssetRegistry.emplace(meta.GetUUID(), std::move(registryTerm));
                        break;
                    }
                    case AssetType::Sampler:
                    {
                        SamplerAssetMeta meta = SamplerAssetMeta::CreateDefault();
                        WriteMetaToFile(meta, metaPath);
                        registryTerm->uuid = meta.GetUUID();
                        std::unique_lock registryLock(m_AssetRegistryMutex);
                        m_AssetRegistry.emplace(meta.GetUUID(), std::move(registryTerm));
                        break;
                    }
                    case AssetType::RenderTexture:
                    {
                        RenderTextureAssetMeta meta = RenderTextureAssetMeta::CreateDefault();
                        WriteMetaToFile(meta, metaPath);
                        registryTerm->uuid = meta.GetUUID();
                        std::unique_lock registryLock(m_AssetRegistryMutex);
                        m_AssetRegistry.emplace(meta.GetUUID(), std::move(registryTerm));
                        break;
                    }
                    case AssetType::ComputeShader:
                    {
                        ComputeShaderAssetMeta meta = ComputeShaderAssetMeta::CreateDefault();
                        WriteMetaToFile(meta, metaPath);
                        registryTerm->uuid = meta.GetUUID();
                        std::unique_lock registryLock(m_AssetRegistryMutex);
                        m_AssetRegistry.emplace(meta.GetUUID(), std::move(registryTerm));
                        break;
                    }
                    case AssetType::Mesh:
                    {
                        MeshAssetMeta meta = MeshAssetMeta::CreateDefault();
                        WriteMetaToFile(meta, metaPath);
                        registryTerm->uuid = meta.GetUUID();
                        std::unique_lock registryLock(m_AssetRegistryMutex);
                        m_AssetRegistry.emplace(meta.GetUUID(), std::move(registryTerm));
                        break;
                    }
                    case AssetType::Material:
                    {
                        MaterialAssetMeta meta = MaterialAssetMeta::CreateDefault();
                        WriteMetaToFile(meta, metaPath);
                        registryTerm->uuid = meta.GetUUID();
                        std::unique_lock registryLock(m_AssetRegistryMutex);
                        m_AssetRegistry.emplace(meta.GetUUID(), std::move(registryTerm));
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    AssetType AssetManager::GetAssetType(UUID uuid) const
    {
        std::unique_lock registryLock(m_AssetRegistryMutex);
        if (!m_AssetRegistry.contains(uuid))
        {
            return AssetType::Unknown;
        }

        return m_AssetRegistry.at(uuid)->type;
    }

    bool AssetManager::HasAsset(UUID uuid) const
    {
        std::unique_lock registryLock(m_AssetRegistryMutex);
        return m_AssetRegistry.contains(uuid);
    }

    std::filesystem::path AssetManager::GetAssetPath(UUID uuid) const
    {
        std::unique_lock registryLock(m_AssetRegistryMutex);
        if (!m_AssetRegistry.contains(uuid))
        {
            return {};
        }

        return m_AssetRegistry.at(uuid)->filePath;
    }

    std::vector<AssetRegistryTerm*> AssetManager::GetAssetsByType(AssetType type) const
    {
        std::vector<AssetRegistryTerm*> assets;

        std::unique_lock registryLock(m_AssetRegistryMutex);
        for (const auto& registryTerm : m_AssetRegistry | std::views::values)
        {
            if (registryTerm->type == type)
            {
                assets.push_back(registryTerm.get());
            }
        }

        std::ranges::sort(assets,
                          [](const AssetRegistryTerm* lhs, const AssetRegistryTerm* rhs) {
                              return lhs->filePath.filename().string() < rhs->filePath.filename().string();
                          });
        return assets;
    }

    void AssetManager::RegisterAsset(std::unique_ptr<AssetRegistryTerm> term)
    {
        std::unique_lock registryLock(m_AssetRegistryMutex);
        m_AssetRegistry.emplace(term->uuid, std::move(term));
    }

    void AssetManager::ClearLoadedAssets()
    {
        {
            std::unique_lock assetLock(m_AssetMutex);
            m_Assets.clear();
        }

        {
            std::unique_lock dependencyLock(m_DependencyMutex);
            m_Dependencies.clear();
            m_Dependents.clear();
        }

        std::unique_lock registryLock(m_AssetRegistryMutex);
        for (auto& registryTerm : m_AssetRegistry | std::views::values)
        {
            registryTerm->state = AssetState::Unloaded;
        }
    }

    Asset* AssetManager::RequestAsset(UUID uuid)
    {
        {
            std::unique_lock assetLock(m_AssetMutex);
            if (m_Assets.contains(uuid))
            {
                return m_Assets[uuid].get();
            }
        }

        AssetRegistryTerm* registry = nullptr;
        {
            std::unique_lock registryLock(m_AssetRegistryMutex);
            if (!m_AssetRegistry.contains(uuid))
            {
                return nullptr;
            }

            registry = m_AssetRegistry.at(uuid).get();
        }

        {
            std::unique_lock registryStateLock(registry->mutex);
            auto state = registry->state;
            if (state == AssetState::Failed)
            {
                return nullptr;
            }

            if (state == AssetState::Loaded)
            {
                registryStateLock.unlock();
                std::unique_lock assetLock(m_AssetMutex);
                if (m_Assets.contains(uuid))
                {
                    return m_Assets[uuid].get();
                }
                return nullptr;
            }

            if (state == AssetState::Loading)
            {
                return nullptr;
            }

            registry->state = AssetState::Loading;
        }

        std::thread([this, uuid, registry] {
            LoadAssetFromRegistry(uuid, registry);
        }).detach();
        return nullptr;
    }

    Asset* AssetManager::RequestAssetBlocked(UUID uuid)
    {
        {
            std::unique_lock assetLock(m_AssetMutex);
            if (m_Assets.contains(uuid))
            {
                return m_Assets[uuid].get();
            }
        }

        AssetRegistryTerm* registry = nullptr;
        {
            std::unique_lock registryLock(m_AssetRegistryMutex);
            if (!m_AssetRegistry.contains(uuid))
            {
                return nullptr;
            }

            registry = m_AssetRegistry.at(uuid).get();
        }

        while (true)
        {
            std::unique_lock registryStateLock(registry->mutex);
            auto state = registry->state;
            if (state == AssetState::Failed)
            {
                return nullptr;
            }

            if (state == AssetState::Loaded)
            {
                registryStateLock.unlock();
                std::unique_lock assetLock(m_AssetMutex);
                if (m_Assets.contains(uuid))
                {
                    return m_Assets[uuid].get();
                }
                return nullptr;
            }

            if (state == AssetState::Loading)
            {
                registry->loadingCondition.wait(registryStateLock,
                                                [registry] {
                                                    return registry->state != AssetState::Loading;
                                                });
                continue;
            }

            if (state != AssetState::Unloaded)
            {
                continue;
            }

            registry->state = AssetState::Loading;
            registryStateLock.unlock();
            return LoadAssetFromRegistry(uuid, registry);
        }
    }

    Asset* AssetManager::LoadAssetFromRegistry(UUID uuid, AssetRegistryTerm* registry)
    {
        std::unique_ptr<Asset> asset = nullptr;

        switch (registry->type)
        {
            case AssetType::Texture:
                asset = AssetImporter::ImportTexture(registry);
                break;
            case AssetType::Shader:
                asset = AssetImporter::ImportShader(registry);
                break;
            case AssetType::Sampler:
                asset = AssetImporter::ImportSampler(registry);
                break;
            case AssetType::RenderTexture:
                asset = AssetImporter::ImportRenderTexture(registry);
                break;
            case AssetType::ComputeShader:
                asset = AssetImporter::ImportComputeShader(registry);
                break;
            case AssetType::Mesh:
                asset = AssetImporter::ImportMesh(registry);
                break;
            case AssetType::Material:
                asset = AssetImporter::ImportMaterial(this, registry);
                break;
            default:
                break;
        }

        if (asset)
        {
            if (registry->type == AssetType::Material)
            {
                auto* materialAsset = static_cast<MaterialAsset*>(asset.get());
                auto materialUUID = materialAsset->GetUUID();

                std::unique_lock dependencyLock(m_DependencyMutex);
                m_Dependencies[materialUUID].clear();

                auto shaderUUID = materialAsset->GetMeta().GetShader();
                if (shaderUUID != UUID(-1))
                {
                    m_Dependencies[materialUUID].insert(shaderUUID);
                    m_Dependents[shaderUUID].insert(materialUUID);
                }

                for (const auto& property : materialAsset->GetMeta().GetProperties())
                {
                    if (property.sampler != UUID(-1))
                    {
                        auto samplerUUID = property.sampler;
                        m_Dependencies[materialUUID].insert(samplerUUID);
                        m_Dependents[samplerUUID].insert(materialUUID);
                    }

                    if (property.texture != UUID(-1))
                    {
                        auto textureUUID = property.texture;
                        m_Dependencies[materialUUID].insert(textureUUID);
                        m_Dependents[textureUUID].insert(materialUUID);
                    }
                }
            }

            auto assetPointer = asset.get();
            {
                std::unique_lock assetLock(m_AssetMutex);
                m_Assets[uuid] = std::move(asset);
            }

            {
                std::unique_lock loadedStateLock(registry->mutex);
                registry->state = AssetState::Loaded;
            }
            registry->loadingCondition.notify_all();
            return assetPointer;
        }

        {
            std::unique_lock failedStateLock(registry->mutex);
            registry->state = AssetState::Failed;
        }
        registry->loadingCondition.notify_all();
        return nullptr;
    }
} // namespace Hazel