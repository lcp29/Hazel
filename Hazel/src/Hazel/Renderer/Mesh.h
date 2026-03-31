//
// Created by helmholtz on 2026/3/28.
//

#pragma once
#include "Hazel/Asset/MeshAsset.h"
#include <vector>

namespace Hazel
{
    class Mesh
    {
    public:
        Mesh() = delete;

        Mesh(UUID uuid, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
            : m_UUID(uuid), m_IsValid(true), m_Vertices(vertices), m_Indices(indices) {}

        Mesh(UUID uuid,
             const std::vector<Vertex>& vertices,
             const std::vector<uint32_t>& indices,
             const std::vector<MeshletInfo>& meshlets)
            : m_UUID(uuid), m_IsValid(true), m_Vertices(vertices), m_Indices(indices), m_Meshlets(meshlets),
              m_HasMeshlets(true) {}

        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;
        ~Mesh();

        void Release();
        void ReleaseImmediate();

    private:
        UUID m_UUID = 0;
        bool m_IsValid = false;
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
        std::vector<MeshletInfo> m_Meshlets;
        bool m_HasMeshlets = false;
    };
} // Hazel