// Implements mesh asset support.
// Created: 2026-04-01.

#include "MeshAsset.h"

namespace Aster
{
    YAML::Node MeshAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(m_UUID);
        rootNode["Version"] = m_Version;
        rootNode["GenerateMeshlets"] = m_GenerateMeshlets;
        return rootNode;
    }

    MeshAssetMeta MeshAssetMeta::Deserialize(const YAML::Node& node)
    {
        MeshAssetMeta meta;
        meta.m_UUID = node["UUID"] ? Hazel::UUID(node["UUID"].as<uint64_t>()) : Hazel::UUID();
        meta.m_Version = node["Version"] ? node["Version"].as<uint64_t>() : 0;
        meta.m_GenerateMeshlets = node["GenerateMeshlets"] ? node["GenerateMeshlets"].as<bool>() : false;
        return meta;
    }

    MeshAssetMeta MeshAssetMeta::CreateDefault()
    {
        MeshAssetMeta meta;
        meta.m_UUID = Hazel::UUID();
        meta.m_Version = 0;
        meta.m_GenerateMeshlets = false;
        return meta;
    }
} // namespace Aster
