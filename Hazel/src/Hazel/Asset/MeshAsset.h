//
// Created by helmholtz on 2026/4/1.
//

#pragma once
#include "Asset.h"

#include <filesystem>
#include <glm/glm.hpp>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec3 normal;
        glm::vec3 tangent;
    };

    struct MeshletInfo
    {
        uint32_t indexStart = 0;
        uint32_t indexCount = 0;
        uint32_t vertexStart = 0;
        uint32_t vertexCount = 0;
    };

    struct MeshAssetMeta
    {
        YAML::Node Serialize() const;
        static MeshAssetMeta Deserialize(const YAML::Node& node);
        static MeshAssetMeta CreateDefault();

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

        bool GenerateMeshlets() const
        {
            return m_GenerateMeshlets;
        }

        void SetGenerateMeshlets(bool generateMeshlets)
        {
            if (m_GenerateMeshlets == generateMeshlets)
            {
                return;
            }
            m_GenerateMeshlets = generateMeshlets;
            VersionUp();
        }

    private:
        UUID m_UUID = 0;
        bool m_GenerateMeshlets = false;
        uint64_t m_Version = 0;
    };

    struct MeshAssetData
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MeshletInfo> meshlets;
    };

    class MeshAsset : public Asset
    {
    public:
        MeshAsset() = delete;

        MeshAsset(AssetRegistryTerm* registryTerm,
                  const MeshAssetMeta& meta,
                  MeshAssetData data)
            : Asset(registryTerm), m_Meta(meta), m_Data(std::move(data)) {}

        uint64_t GetVersion() const final
        {
            return m_Meta.GetVersion();
        }

        void VersionUp() final
        {
            m_Meta.VersionUp();
        }

        const MeshAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        MeshAssetMeta& GetMeta()
        {
            return m_Meta;
        }

        const MeshAssetData& GetData() const
        {
            return m_Data;
        }

        MeshAssetData& GetData()
        {
            return m_Data;
        }

    private:
        MeshAssetMeta m_Meta{};
        MeshAssetData m_Data{};
    };
} // Hazel
