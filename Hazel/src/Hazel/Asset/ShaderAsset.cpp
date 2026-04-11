//
// Created by helmholtz on 2026/3/31.
//

#include "ShaderAsset.h"

namespace Hazel
{
    YAML::Node ShaderAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(m_UUID);
        rootNode["Version"] = m_Version;
        return rootNode;
    }

    ShaderAssetMeta ShaderAssetMeta::Deserialize(const YAML::Node& node)
    {
        ShaderAssetMeta meta;
        meta.m_UUID = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        meta.m_Version = node["Version"] ? node["Version"].as<uint64_t>() : 0;
        return meta;
    }

    ShaderAssetMeta ShaderAssetMeta::CreateDefault()
    {
        ShaderAssetMeta meta;
        meta.m_UUID = UUID();
        meta.m_Version = 0;
        return meta;
    }
} // namespace Hazel