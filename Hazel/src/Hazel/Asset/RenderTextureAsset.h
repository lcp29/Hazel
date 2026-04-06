//
// Created by helmholtz on 2026/3/24.
//

#pragma once

#include "Asset.h"
#include "Hazel/Renderer/GPUAsset/GPURenderTextureAsset.h"

#include <yaml-cpp/yaml.h>

namespace Hazel
{
    struct RenderTextureAssetMeta
    {
        YAML::Node Serialize() const;
        static RenderTextureAssetMeta Deserialize(const YAML::Node& node);
        static RenderTextureAssetMeta CreateDefault();

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

        const RenderTextureDesc& GetDesc() const
        {
            return m_Desc;
        }

        void SetWidth(uint32_t width)
        {
            if (m_Desc.width == width)
            {
                return;
            }
            m_Desc.width = width;
            VersionUp();
        }

        void SetHeight(uint32_t height)
        {
            if (m_Desc.height == height)
            {
                return;
            }
            m_Desc.height = height;
            VersionUp();
        }

        void SetDepth(uint32_t depth)
        {
            if (m_Desc.depth == depth)
            {
                return;
            }
            m_Desc.depth = depth;
            VersionUp();
        }

        void SetArrayLayers(uint32_t arrayLayers)
        {
            if (m_Desc.arrayLayers == arrayLayers)
            {
                return;
            }
            m_Desc.arrayLayers = arrayLayers;
            VersionUp();
        }

        void SetViewType(RHIImageViewType viewType)
        {
            if (m_Desc.viewType == viewType)
            {
                return;
            }
            m_Desc.viewType = viewType;
            VersionUp();
        }

        void SetUseMipmap(bool useMipmap)
        {
            if (m_Desc.useMipmap == useMipmap)
            {
                return;
            }
            m_Desc.useMipmap = useMipmap;
            VersionUp();
        }

        void SetPerFrame(bool perFrame)
        {
            if (m_Desc.perFrame == perFrame)
            {
                return;
            }
            m_Desc.perFrame = perFrame;
            VersionUp();
        }

        void SetFormat(RHIFormat format)
        {
            if (m_Desc.format == format)
            {
                return;
            }
            m_Desc.format = format;
            VersionUp();
        }

        void SetUsages(RHIImageUsages usages)
        {
            if (m_Desc.usages == usages)
            {
                return;
            }
            m_Desc.usages = usages;
            VersionUp();
        }

    private:
        UUID m_UUID = 0;
        uint64_t m_Version = 0;
        RenderTextureDesc m_Desc{};
    };

    class RenderTextureAsset : public Asset
    {
    public:
        RenderTextureAsset() = delete;

        RenderTextureAsset(AssetRegistryTerm* registryTerm, const RenderTextureAssetMeta& meta)
            : Asset(registryTerm), m_Meta(meta) {}

        uint64_t GetVersion() const final
        {
            return m_Meta.GetVersion();
        }

        void VersionUp() final
        {
            m_Meta.VersionUp();
        }

        const RenderTextureAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        RenderTextureAssetMeta& GetMeta()
        {
            return m_Meta;
        }

    private:
        RenderTextureAssetMeta m_Meta{};
    };
} // namespace Hazel
