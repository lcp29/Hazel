//
// Created by helmholtz on 2026/3/23.
//

#include "AssetManager.h"

#include "Hazel/Project/Project.h"
#include "Hazel/Renderer/Renderer.h"

#include <ranges>

namespace Hazel
{
    AssetManager::AssetManager(Project* project, Renderer* renderer)
        : m_Project(project),
          m_Renderer(renderer) {}

    AssetManager::AssetManager(AssetManager&& other) noexcept
    {
        m_Project = other.m_Project;
        m_Renderer = other.m_Renderer;
        m_Textures = std::move(other.m_Textures);
        m_ComputeShaders = std::move(other.m_ComputeShaders);
        m_Meshes = std::move(other.m_Meshes);
        m_Shaders = std::move(other.m_Shaders);
        m_RenderTextures = std::move(other.m_RenderTextures);
        m_Samplers = std::move(other.m_Samplers);
        m_AssetTypes = std::move(other.m_AssetTypes);

        other.m_Project = nullptr;
        other.m_Renderer = nullptr;
    }

    AssetManager& AssetManager::operator=(AssetManager&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        m_Project = other.m_Project;
        m_Renderer = other.m_Renderer;
        m_Textures = std::move(other.m_Textures);
        m_ComputeShaders = std::move(other.m_ComputeShaders);
        m_Meshes = std::move(other.m_Meshes);
        m_Shaders = std::move(other.m_Shaders);
        m_RenderTextures = std::move(other.m_RenderTextures);
        m_Samplers = std::move(other.m_Samplers);
        m_AssetTypes = std::move(other.m_AssetTypes);

        other.m_Project = nullptr;
        other.m_Renderer = nullptr;
        return *this;
    }

    void AssetManager::ScanAll()
    {
        auto assetDirectory = m_Project->GetAssetDirectory();

        for (const auto& file : std::filesystem::recursive_directory_iterator(assetDirectory))
        {
            if (file.is_regular_file())
            {
                const auto& path = file.path();
                const auto extension = path.extension().string();

                static const std::unordered_set<std::string> imageExtensions =
                    {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".hdr"};

                if (imageExtensions.contains(extension))
                {
                    auto metaFileName = path.string() + ".meta";
                    TextureAssetMeta meta;
                    if (std::filesystem::exists(metaFileName))
                    {
                        YAML::Node metaNode = YAML::LoadFile(metaFileName);
                        meta = TextureAssetMeta::Deserialize(metaNode);
                    }
                    else
                    {
                        meta.uuid = UUID();
                        auto metaNode = meta.Serialize();
                        std::ofstream output(metaFileName);
                        output << metaNode;
                        output.close();
                    }
                    if (m_Textures.contains(meta.uuid))
                    {
                        continue;
                    }
                    TextureAsset asset(meta.uuid, path, m_Renderer, meta);
                    m_Textures.emplace(meta.uuid, std::move(asset));
                    m_AssetTypes.emplace(meta.uuid, AssetType::Texture);
                }
                else if (extension == ".comp")
                {
                    auto metaFileName = path.string() + ".meta";
                    ComputeShaderAssetMeta meta;
                    if (std::filesystem::exists(metaFileName))
                    {
                        YAML::Node metaNode = YAML::LoadFile(metaFileName);
                        meta = ComputeShaderAssetMeta::Deserialize(metaNode);
                    }
                    else
                    {
                        meta.uuid = UUID();
                        auto metaNode = meta.Serialize();
                        std::ofstream output(metaFileName);
                        output << metaNode;
                        output.close();
                    }
                    if (m_ComputeShaders.contains(meta.uuid))
                    {
                        continue;
                    }
                    ComputeShaderAsset asset(m_Renderer, path, meta);
                    m_ComputeShaders.emplace(meta.uuid, std::move(asset));
                    m_AssetTypes.emplace(meta.uuid, AssetType::ComputeShader);
                }
                else if (extension == ".obj")
                {
                    auto metaFileName = path.string() + ".meta";
                    MeshAssetMeta meta;
                    if (std::filesystem::exists(metaFileName))
                    {
                        YAML::Node metaNode = YAML::LoadFile(metaFileName);
                        meta = MeshAssetMeta::Deserialize(metaNode);
                    }
                    else
                    {
                        meta.uuid = UUID();
                        auto metaNode = meta.Serialize();
                        std::ofstream output(metaFileName);
                        output << metaNode;
                        output.close();
                    }
                    if (m_Meshes.contains(meta.uuid))
                    {
                        continue;
                    }
                    MeshAsset asset(meta.uuid, m_Renderer, path, meta);
                    m_Meshes.emplace(meta.uuid, std::move(asset));
                    m_AssetTypes.emplace(meta.uuid, AssetType::Mesh);
                }
                else if (extension == ".shader")
                {
                    auto metaFileName = path.string() + ".meta";
                    ShaderAssetMeta meta;
                    if (std::filesystem::exists(metaFileName))
                    {
                        YAML::Node metaNode = YAML::LoadFile(metaFileName);
                        meta = ShaderAssetMeta::Deserialize(metaNode);
                    }
                    else
                    {
                        meta.uuid = UUID();
                        auto metaNode = meta.Serialize();
                        std::ofstream output(metaFileName);
                        output << metaNode;
                        output.close();
                    }
                    if (m_Shaders.contains(meta.uuid))
                    {
                        continue;
                    }
                    ShaderAsset asset(m_Renderer, path, meta);
                    m_Shaders.emplace(meta.uuid, std::move(asset));
                    m_AssetTypes.emplace(meta.uuid, AssetType::Shader);
                }
                else if (extension == ".meta")
                {
                    const auto stemFilePath = path.stem();
                    const auto stemExtension = stemFilePath.extension().string();
                    if (stemExtension == ".rt")
                    {
                        YAML::Node metaNode = YAML::LoadFile(path.string());
                        auto meta = RenderTextureAssetMeta::Deserialize(metaNode);
                        // new meta file
                        if (!metaNode["UUID"])
                        {
                            metaNode = meta.Serialize();
                            std::ofstream output(path.string());
                            output << metaNode;
                            output.close();
                        }
                        if (m_RenderTextures.contains(meta.uuid))
                        {
                            continue;
                        }
                        RenderTextureAsset asset(meta.uuid, path, m_Renderer, meta);
                        m_RenderTextures.emplace(meta.uuid, std::move(asset));
                        m_AssetTypes.emplace(meta.uuid, AssetType::RenderTexture);
                    }
                    else if (stemExtension == ".sampler")
                    {
                        YAML::Node metaNode = YAML::LoadFile(path.string());
                        auto meta = SamplerAssetMeta::Deserialize(metaNode);
                        // new meta file
                        if (!metaNode["UUID"])
                        {
                            metaNode = meta.Serialize();
                            std::ofstream output(path.string());
                            output << metaNode;
                            output.close();
                        }
                        if (m_Samplers.contains(meta.uuid))
                        {
                            continue;
                        }
                        SamplerAsset asset(meta.uuid, path, m_Renderer, meta);
                        m_Samplers.emplace(meta.uuid, std::move(asset));
                        m_AssetTypes.emplace(meta.uuid, AssetType::Sampler);
                    }
                    else if (stemExtension == ".mat")
                    {
                        YAML::Node metaNode = YAML::LoadFile(path.string());
                        auto meta = MaterialAssetMeta::Deserialize(metaNode);
                        // new meta file
                        if (!metaNode["UUID"])
                        {
                            metaNode = meta.Serialize();
                            std::ofstream output(path.string());
                            output << metaNode;
                            output.close();
                        }
                        if (m_Materials.contains(meta.uuid))
                        {
                            continue;
                        }
                        MaterialAsset asset(meta.uuid, this, m_Renderer, path, meta);
                        m_Materials.emplace(meta.uuid, std::move(asset));
                        m_AssetTypes.emplace(meta.uuid, AssetType::Material);
                    }
                }
            }
        }
    }

    void AssetManager::WriteAllMetaFiles() const
    {
        // textures
        for (const auto& asset : m_Textures | std::views::values)
        {
            auto metaNode = asset.GetMeta().Serialize();
            std::ofstream output(asset.GetFilePath().string() + ".meta");
            output << metaNode;
            output.close();
        }

        // compute shaders
        for (const auto& asset : m_ComputeShaders | std::views::values)
        {
            auto metaNode = asset.GetMeta().Serialize();
            std::ofstream output(asset.GetFilePath().string() + ".meta");
            output << metaNode;
            output.close();
        }

        // shaders
        for (const auto& asset : m_Shaders | std::views::values)
        {
            auto metaNode = asset.GetMeta().Serialize();
            std::ofstream output(asset.GetFilePath().string() + ".meta");
            output << metaNode;
            output.close();
        }

        // render textures
        for (const auto& asset : m_RenderTextures | std::views::values)
        {
            auto metaNode = asset.GetMeta().Serialize();
            std::ofstream output(asset.GetFilePath().string());
            output << metaNode;
            output.close();
        }

        // samplers
        for (const auto& asset : m_Samplers | std::views::values)
        {
            auto metaNode = asset.GetMeta().Serialize();
            std::ofstream output(asset.GetFilePath().string());
            output << metaNode;
            output.close();
        }

        // meshes
        for (const auto& asset : m_Meshes | std::views::values)
        {
            auto metaNode = asset.GetMeta().Serialize();
            std::ofstream output(asset.GetFilePath().string() + ".meta");
            output << metaNode;
            output.close();
        }

        // materials
        for (const auto& asset : m_Materials | std::views::values)
        {
            auto metaNode = asset.GetMeta().Serialize();
            std::ofstream output(asset.GetFilePath().string());
            output << metaNode;
            output.close();
        }
    }

    void AssetManager::UnloadAllAssets()
    {
        // materials 'go' before shaders
        for (auto& asset : m_Materials | std::views::values)
        {
            asset.Unload();
        }
        for (auto& asset : m_Shaders | std::views::values)
        {
            asset.Unload();
        }

        for (auto& asset : m_Textures | std::views::values)
        {
            asset.Unload();
        }
        for (auto& asset : m_ComputeShaders | std::views::values)
        {
            asset.Unload();
        }
        for (auto& asset : m_RenderTextures | std::views::values)
        {
            asset.Unload();
        }
        for (auto& asset : m_Samplers | std::views::values)
        {
            asset.Unload();
        }
        for (auto& asset : m_Meshes | std::views::values)
        {
            asset.Unload();
        }
    }

    void AssetManager::LoadAssetUUIDs(const std::vector<UUID>& uuids)
    {
        for (auto& uuid : uuids)
        {
            if (uuid == UUID(-1))
            {
                continue;
            }

            auto type = m_AssetTypes[uuid];
            switch (type)
            {
                case AssetType::Texture:
                    if (m_Textures.contains(uuid))
                    {
                        m_Textures.at(uuid).Load();
                    }
                    break;
                case AssetType::ComputeShader:
                    if (m_ComputeShaders.contains(uuid))
                    {
                        m_ComputeShaders.at(uuid).Load();
                    }
                    break;
                case AssetType::Shader:
                    if (m_Shaders.contains(uuid))
                    {
                        m_Shaders.at(uuid).Load();
                    }
                    break;
                case AssetType::RenderTexture:
                    if (m_RenderTextures.contains(uuid))
                    {
                        m_RenderTextures.at(uuid).Load();
                    }
                    break;
                case AssetType::Sampler:
                    if (m_Samplers.contains(uuid))
                    {
                        m_Samplers.at(uuid).Load();
                    }
                    break;
                case AssetType::Mesh:
                    if (m_Meshes.contains(uuid))
                    {
                        m_Meshes.at(uuid).Load();
                    }
                    break;
                case AssetType::Material:
                    if (m_Materials.contains(uuid))
                    {
                        auto& mat = m_Materials.at(uuid);
                        mat.Load();
                        auto shaderUUID = mat.GetMeta().shader;
                        if (m_Shaders.contains(shaderUUID))
                        {
                            auto& shader = m_Shaders.at(mat.GetMeta().shader);
                            shader.GetShader()->RecreateMaterialResourceGroup();
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
} // namespace Hazel