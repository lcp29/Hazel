//
// Created by helmholtz on 2026/3/28.
//

#pragma once
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"
#include "Hazel/Asset/MeshAsset.h"
#include <vector>

namespace Hazel
{
    class Renderer;

    class GPUMeshAsset : public GPUAsset
    {
    public:
        GPUMeshAsset() = delete;

        GPUMeshAsset(const UUID uuid,
                     uint64_t sourceVersion,
                     Renderer* renderer,
                     const std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices,
                     const std::vector<MeshletInfo>& meshlets,
                     uint64_t lastReferencedFrame = 0)
            : GPUAsset(uuid, AssetType::Mesh, renderer, sourceVersion, lastReferencedFrame),
              m_IsValid(true),
              m_Vertices(vertices),
              m_Indices(indices),
              m_Meshlets(meshlets),
              m_HasMeshlets(!meshlets.empty()) {}

        void Release() override;
        void ReleaseImmediate() override;

    private:
        bool m_IsValid = false;
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
        std::vector<MeshletInfo> m_Meshlets;
        bool m_HasMeshlets = false;
    };
} // Hazel