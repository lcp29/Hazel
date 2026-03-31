//
// Created by helmholtz on 2026/3/28.
//

#include "Mesh.h"

#include <algorithm>
#include <vector>

namespace Hazel
{
    Mesh::Mesh(Mesh&& other) noexcept
        : m_UUID(other.m_UUID),
          m_IsValid(other.m_IsValid),
          m_Vertices(std::move(other.m_Vertices)),
          m_Indices(std::move(other.m_Indices)),
          m_Meshlets(std::move(other.m_Meshlets)),
          m_HasMeshlets(other.m_HasMeshlets)
    {
        other.m_IsValid = false;
        other.m_HasMeshlets = false;
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        m_UUID = other.m_UUID;
        m_IsValid = other.m_IsValid;
        m_Vertices = std::move(other.m_Vertices);
        m_Indices = std::move(other.m_Indices);
        m_Meshlets = std::move(other.m_Meshlets);
        m_HasMeshlets = other.m_HasMeshlets;
        other.m_HasMeshlets = false;
        return *this;
    }

    Mesh::~Mesh() = default;

    void Mesh::Release() {}
    void Mesh::ReleaseImmediate() {}
} // Hazel