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
    }
} // namespace Hazel