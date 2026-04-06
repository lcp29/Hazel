//
// Created by helmholtz on 2026/3/24.
//

#pragma once

#include "Asset.h"
#include "AssetManager.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/GPUAsset/GPUTextureAsset.h"

#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Renderer;

    struct TextureAssetMeta
    {
        YAML::Node Serialize() const;
        static TextureAssetMeta Deserialize(const YAML::Node& node);
        static TextureAssetMeta CreateDefault();

        UUID GetUUID() const
        {
            return m_UUID;
        }

        uint64_t GetVersion() const
        {
            return m_Version;
        }

        void VersionUp()
        {
            m_Version++;
        }

        bool IsSRGB() const
        {
            return m_IsSRGB;
        }

        void SetSRGB(bool isSRGB)
        {
            if (m_IsSRGB == isSRGB)
            {
                return;
            }
            m_IsSRGB = isSRGB;
            m_Version++;
        }

        bool UseMipmap() const
        {
            return m_UseMipmap;
        }

        void SetUseMipmap(bool useMipmap)
        {
            if (m_UseMipmap == useMipmap)
            {
                return;
            }
            m_UseMipmap = useMipmap;
            m_Version++;
        }

        bool AllowStorageLoad() const
        {
            return m_AllowStorageLoad;
        }

        void SetAllowStorageLoad(bool allowStorageLoad)
        {
            if (m_AllowStorageLoad == allowStorageLoad)
            {
                return;
            }
            m_AllowStorageLoad = allowStorageLoad;
            m_Version++;
        }

    private:
        UUID m_UUID = 0;
        uint64_t m_Version = 0;
        bool m_IsSRGB = true;
        bool m_UseMipmap = false;
        bool m_AllowStorageLoad = false;
    };

    struct TextureAssetData
    {
        uint32_t width = 0;
        uint32_t height = 0;
        RHIFormat format = RHIFormat::RGBA8SRGB;
        std::vector<uint8_t> rawImageData;
    };

    class TextureAsset : public Asset
    {
    public:
        TextureAsset() = delete;

        TextureAsset(AssetRegistryTerm* registryTerm,
                     const TextureAssetMeta& meta,
                     TextureAssetData textureData)
            : Asset(registryTerm), m_Meta(meta), m_TextureData(std::move(textureData)) {}

        uint64_t GetVersion() const final
        {
            return m_Meta.GetVersion();
        }

        void VersionUp() final
        {
            m_Meta.VersionUp();
        }

        const TextureAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        TextureAssetMeta& GetMeta()
        {
            return m_Meta;
        }

        const TextureAssetData& GetTextureData() const
        {
            return m_TextureData;
        }

        TextureAssetData& GetTextureData()
        {
            return m_TextureData;
        }

    private:
        TextureAssetMeta m_Meta{};
        TextureAssetData m_TextureData{};
    };
} // namespace Hazel