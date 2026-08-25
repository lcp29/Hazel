// Implements compute shader asset support.
// Created: 2026-03-29.

#include "ComputeShaderAsset.h"

namespace Aster
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
        meta.m_UUID = node["UUID"] ? Hazel::UUID(node["UUID"].as<uint64_t>()) : Hazel::UUID();
        meta.m_Version = node["Version"] ? node["Version"].as<uint64_t>() : 0;
        return meta;
    }

    ComputeShaderAssetMeta ComputeShaderAssetMeta::CreateDefault()
    {
        ComputeShaderAssetMeta meta;
        meta.m_UUID = Hazel::UUID();
        meta.m_Version = 0;
        return meta;
    }
} // namespace Aster
