//
// Created by helmholtz on 2026/3/28.
//

#pragma once
#include "Hazel/Asset/MeshAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"
#include "Hazel/Renderer/GPUStructure.h"

#include <vector>

namespace Hazel
{
    class Renderer;

    struct alignas(16) GPUVertex
    {
        glm::vec3 position;
        float uv0;
        glm::vec3 normal;
        float uv1;
        glm::vec3 tangent;

        bool operator==(const GPUVertex& other) const
        {
            return position == other.position && normal == other.normal && tangent == other.tangent && uv0 == other.uv0
                   && uv1 == other.uv1;
        }

        GPUVertex operator()(const Vertex& vertex)
        {
            return GPUVertex{
                vertex.position,
                vertex.texCoord.x,
                vertex.normal,
                vertex.texCoord.y,
                vertex.tangent,
            };
        }

        glm::vec3 GetPosition() const { return position; }

        glm::vec3 GetNormal() const { return normal; }

        glm::vec3 GetTangent() const { return tangent; }

        glm::vec2 GetUV() const { return glm::vec2(uv0, uv1); }

        void SetPosition(const glm::vec3& pos) { position = pos; }

        void SetNormal(const glm::vec3& norm) { normal = norm; }

        void SetTangent(const glm::vec3& tang) { tangent = tang; }

        void SetUV(const glm::vec2& uv)
        {
            uv0 = uv.x;
            uv1 = uv.y;
        }
    };

    constexpr int kGPUVertexSize = sizeof(GPUVertex);

    struct alignas(16) GPUMeshletInfo
    {
        uint32_t vertexOffset;
        uint32_t vertexCount;
        uint32_t indexOffset;
        uint32_t indexCount;
        glm::vec3 boundingSphereCenter;
        float boundingSphereRadius = 0.0f;
    };

    class GPUMeshAsset : public GPUAsset
    {
      public:
        GPUMeshAsset() = delete;

        GPUMeshAsset(const UUID uuid,
                     uint64_t sourceVersion,
                     Renderer* renderer,
                     const std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices,
                     const std::vector<GPUMeshletInfo>& meshlets,
                     const std::array<glm::vec3, 2>& aabb,
                     const std::pair<glm::vec3, float>& boundingSphere,
                     uint64_t lastReferencedFrame = 0)
            : GPUAsset(uuid, AssetType::Mesh, renderer, sourceVersion, lastReferencedFrame)
            , m_IsValid(true)
            , m_Indices(indices)
            , m_Meshlets(meshlets)
            , m_HasMeshlets(!meshlets.empty())
            , m_AabbMin(aabb[0])
            , m_AabbMax(aabb[1])
            , m_BoundingSphereCenter(boundingSphere.first)
            , m_BoundingSphereRadius(boundingSphere.second)
            , m_Vertices(vertices)
        {}

        ~GPUMeshAsset() override;

        void Release() override;
        void ReleaseImmediate() override;

        std::vector<Vertex>& GetVertices() { return m_Vertices; }

        std::vector<uint32_t>& GetIndices() { return m_Indices; }

        bool HasMeshlets() const { return m_HasMeshlets; }

        const std::vector<GPUMeshletInfo>& GetMeshlets() const { return m_Meshlets; }

        glm::vec3 GetBoundingSphereCenter() const { return m_BoundingSphereCenter; }

        float GetBoundingSphereRadius() const { return m_BoundingSphereRadius; }

        const std::vector<uint32_t>& GetVertexVirtualPages() const { return m_VertexVirtualPages; }

        const std::vector<uint32_t>& GetIndexVirtualPages() const { return m_IndexVirtualPages; }

        const std::vector<uint32_t>& GetMeshletVirtualPages() const { return m_MeshletVirtualPages; }

        void SetVertexVirtualPages(std::vector<uint32_t> pages) { m_VertexVirtualPages = std::move(pages); }

        void SetIndexVirtualPages(std::vector<uint32_t> pages) { m_IndexVirtualPages = std::move(pages); }

        void SetMeshletVirtualPages(std::vector<uint32_t> pages) { m_MeshletVirtualPages = std::move(pages); }

        // TODO: TEMP URGENT INTERVIEW: temporary vertex/index buffer path
        RHIBuffer* GetVertexBuffer() const { return m_VertexBuffer; }

        // TODO: TEMP URGENT INTERVIEW: temporary vertex/index buffer path
        RHIBuffer* GetIndexBuffer() const { return m_IndexBuffer; }

        // TODO: TEMP URGENT INTERVIEW: temporary vertex/index buffer path
        void SetVertexBuffer(RHIBuffer* buffer) { m_VertexBuffer = buffer; }

        // TODO: TEMP URGENT INTERVIEW: temporary vertex/index buffer path
        void SetIndexBuffer(RHIBuffer* buffer) { m_IndexBuffer = buffer; }

      private:
        bool m_IsValid = false;
        // TODO: TEMP URGENT INTERVIEW: use Vertex instead of GPUVertex
        // std::vector<GPUVertex> m_Vertices;
        std::vector<uint32_t> m_VertexVirtualPages;

        std::vector<uint32_t> m_Indices;
        std::vector<uint32_t> m_IndexVirtualPages;

        std::vector<GPUMeshletInfo> m_Meshlets;
        std::vector<uint32_t> m_MeshletVirtualPages;
        bool m_HasMeshlets = false;

        glm::vec3 m_AabbMin{};
        glm::vec3 m_AabbMax{};
        glm::vec3 m_BoundingSphereCenter{};
        float m_BoundingSphereRadius = 0.0f;

        // TODO: TEMP URGENT INTERVIEW: use vertex and index buffers for each mesh
        std::vector<Vertex> m_Vertices;
        RHIBuffer* m_VertexBuffer = nullptr;
        RHIBuffer* m_IndexBuffer = nullptr;
    };
} // namespace Hazel