// Implements texture asset support.
// Created: 2026-03-24.

#include "TextureAsset.h"

#include "Hazel/Renderer/Renderer.h"

namespace Aster
{
    YAML::Node TextureAssetMeta::Serialize() const
    {
        YAML::Node rootNode;

        rootNode["UUID"] = static_cast<uint64_t>(m_UUID);
        rootNode["IsSRGB"] = m_IsSRGB;
        rootNode["UseMipmap"] = m_UseMipmap;
        rootNode["AllowStorageLoad"] = m_AllowStorageLoad;
        rootNode["Version"] = m_Version;

        return rootNode;
    }

    TextureAssetMeta TextureAssetMeta::Deserialize(const YAML::Node& node)
    {
        TextureAssetMeta meta;

        meta.m_UUID = node["UUID"] ? Hazel::UUID(node["UUID"].as<uint64_t>()) : Hazel::UUID();
        meta.m_IsSRGB = node["IsSRGB"] ? node["IsSRGB"].as<bool>() : true;
        meta.m_UseMipmap = node["UseMipmap"] ? node["UseMipmap"].as<bool>() : false;
        meta.m_AllowStorageLoad = node["AllowStorageLoad"] ? node["AllowStorageLoad"].as<bool>() : false;
        meta.m_Version = node["Version"] ? node["Version"].as<uint64_t>() : 0;

        return meta;
    }

    TextureAssetMeta TextureAssetMeta::CreateDefault()
    {
        TextureAssetMeta meta;
        meta.m_UUID = Hazel::UUID();
        meta.m_IsSRGB = true;
        meta.m_UseMipmap = true;
        meta.m_AllowStorageLoad = false;
        meta.m_Version = 0;
        return meta;
    }
} // namespace Aster
