//
// Created by helmholtz on 2026/3/24.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/RenderTexture.h"

#include <filesystem>
#include <optional>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Renderer;

    struct RenderTextureAssetMeta
    {
        UUID uuid = 0;
        RenderTextureDesc desc{};

        YAML::Node Serialize() const;
        static RenderTextureAssetMeta Deserialize(const YAML::Node& node);
    };

    class RenderTextureAsset
    {
    public:
        RenderTextureAsset() = delete;

        RenderTextureAsset(UUID uuid,
                           std::filesystem::path filePath,
                           Renderer* renderer,
                           RenderTextureAssetMeta desc)
            : m_UUID(uuid)
              , m_MetaPath(std::move(filePath))
              , m_Renderer(renderer)
              , m_Meta(desc) {}

        RenderTextureAsset(const RenderTextureAsset&) = delete;
        RenderTextureAsset& operator=(const RenderTextureAsset&) = delete;
        RenderTextureAsset(RenderTextureAsset&& other) noexcept;
        RenderTextureAsset& operator=(RenderTextureAsset&& other) noexcept;
        ~RenderTextureAsset();

        void Load();
        void Unload();

        UUID GetUUID() const
        {
            return m_UUID;
        }

        const std::filesystem::path& GetFilePath() const
        {
            return m_MetaPath;
        }

        const RenderTextureAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        RenderTexture* GetRenderTexture() const
        {
            return m_RenderTexture;
        }

        bool IsValid() const
        {
            return m_RenderTexture && m_RenderTexture->IsValid();
        }

        void Release();

    private:
        UUID m_UUID = 0;
        bool m_IsLoaded = false;
        std::filesystem::path m_MetaPath;
        Renderer* m_Renderer = nullptr;
        RenderTextureAssetMeta m_Meta{};
        RenderTexture* m_RenderTexture = nullptr;
    };
} // namespace Hazel