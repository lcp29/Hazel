//
// Created by helmholtz on 2026/3/24.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/Texture.h"

#include <filesystem>
#include <optional>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Renderer;

    struct TextureAssetMeta
    {
        UUID uuid = 0;
        bool isSRGB = true;
        bool useMipmap = false;

        YAML::Node Serialize() const;
        static TextureAssetMeta Deserialize(const YAML::Node& node);
    };

    class TextureAsset
    {
    public:
        TextureAsset() = default;

        TextureAsset(UUID uuid,
                     std::filesystem::path filePath,
                     Renderer* renderer,
                     TextureAssetMeta meta)
            : m_UUID(uuid)
              , m_FilePath(std::move(filePath))
              , m_Renderer(renderer)
              , m_Meta(meta) {}

        TextureAsset(const TextureAsset&) = delete;
        TextureAsset& operator=(const TextureAsset&) = delete;
        TextureAsset(TextureAsset&& other) noexcept;
        TextureAsset& operator=(TextureAsset&& other) noexcept;
        ~TextureAsset();

        void Load();
        void Unload();

        UUID GetUUID() const
        {
            return m_UUID;
        }

        const std::filesystem::path& GetFilePath() const
        {
            return m_FilePath;
        }

        const TextureAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        Texture* GetTexture() const
        {
            return m_Texture;
        }

        bool IsLoaded() const
        {
            return m_IsLoaded;
        }

        void Release();

    private:
        UUID m_UUID = 0;
        bool m_IsLoaded = false;
        std::filesystem::path m_FilePath;
        Renderer* m_Renderer = nullptr;
        TextureAssetMeta m_Meta{};
        Texture* m_Texture = nullptr;
    };
} // namespace Hazel