//
// Created by helmholtz on 2026/3/28.
//

#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
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

    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        Mesh(const std::vector<Vertex>& vertices,
             const std::vector<uint32_t>& indices,
             const std::vector<MeshletInfo>& meshlets);
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;
        ~Mesh();

        static Mesh CreateFromObj(std::filesystem::path filePath,
                                  bool generateMeshlets = false,
                                  uint32_t maxMeshletVertices = 64,
                                  uint32_t maxMeshletIndices = 126);

    private:
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
        std::vector<MeshletInfo> m_Meshlets;
        bool m_HasMeshlets = false;
    };
} // Hazel