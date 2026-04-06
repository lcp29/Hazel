//
// Created by helmholtz on 2026/3/29.
//

#include "ComputeShaderAsset.h"

namespace Hazel
{
    YAML::Node ComputeShaderAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(m_UUID);
        rootNode["Version"] = m_Version;
        return rootNode;
    }

    ComputeShaderAssetMeta ComputeShaderAssetMeta::Deserialize(const YAML::Node& node)
    {
        ComputeShaderAssetMeta meta;
        meta.m_UUID = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        meta.m_Version = node["Version"] ? node["Version"].as<uint64_t>() : 0;
        return meta;
    }

    ComputeShaderAssetMeta ComputeShaderAssetMeta::CreateDefault()
    {
        ComputeShaderAssetMeta meta;
        meta.m_UUID = UUID();
        meta.m_Version = 0;
        return meta;
    }
} // namespace Hazel
