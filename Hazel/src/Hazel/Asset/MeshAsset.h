//
// Created by helmholtz on 2026/4/1.
//

#pragma once
#include "Hazel/Core/UUID.h"

#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <glm/glm.hpp>

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
        UUID uuid = 0;
        bool generateMeshlets = false;

        YAML::Node Serialize() const;
        static MeshAssetMeta Deserialize(const YAML::Node& node);
    };

    class Mesh;
    class Renderer;

    class MeshAsset
    {
    public:
        MeshAsset() = delete;

        MeshAsset(UUID uuid, Renderer* renderer, std::filesystem::path filePath, MeshAssetMeta meta)
            : m_UUID(uuid), m_Renderer(renderer), m_FilePath(std::move(filePath)), m_Meta(meta)
        {
            LoadMeshFromFile();
        }

        MeshAsset(const MeshAsset&) = delete;
        MeshAsset& operator=(const MeshAsset&) = delete;
        MeshAsset(MeshAsset&& other) noexcept;
        MeshAsset& operator=(MeshAsset&& other) noexcept;
        ~MeshAsset();

        void Load();
        void Unload();

        UUID GetUUID() const
        {
            return m_UUID;
        }

        std::filesystem::path GetFilePath() const
        {
            return m_FilePath;
        }

        const MeshAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        MeshAssetMeta& GetMeta()
        {
            return m_Meta;
        }

        void Recreate();

    private:
        void LoadMeshFromFile();

        UUID m_UUID = 0;
        Renderer* m_Renderer = nullptr;
        bool m_IsLoaded = false;
        std::filesystem::path m_FilePath;
        MeshAssetMeta m_Meta;
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
        std::vector<MeshletInfo> m_Meshlets;
        Mesh* m_Mesh = nullptr;
    };
} // Hazel