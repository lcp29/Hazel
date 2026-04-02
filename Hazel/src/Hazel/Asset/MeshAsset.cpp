//
// Created by helmholtz on 2026/4/1.
//

#include "MeshAsset.h"
#include "Hazel/Core/Log.h"
#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Renderer/Renderer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <queue>

namespace Hazel
{
    namespace
    {
        constexpr float kHardEdgeRadians = glm::radians(45.0f);
        // MSVC doesn't like constexpr here :(
        const float kHardEdgeDotThreshold = std::cos(kHardEdgeRadians);
        constexpr float kDegenerateTriangleEpsilon = 1e-8f;
        constexpr int kMaxMeshletVertices = 64;
        constexpr int kMaxMeshletIndices = 126;

        struct PositionKey
        {
            glm::vec3 value{};

            bool operator==(const PositionKey& other) const
            {
                return value.x == other.value.x && value.y == other.value.y && value.z == other.value.z;
            }
        };

        struct PositionKeyHash
        {
            size_t operator()(const PositionKey& key) const
            {
                const size_t h1 = std::hash<float>{}(key.value.x);
                const size_t h2 = std::hash<float>{}(key.value.y);
                const size_t h3 = std::hash<float>{}(key.value.z);
                return h1 ^ (h2 << 1) ^ (h3 << 2);
            }
        };

        struct EdgeKey
        {
            uint32_t a = 0;
            uint32_t b = 0;

            EdgeKey(uint32_t lhs, uint32_t rhs)
            {
                a = std::min(lhs, rhs);
                b = std::max(lhs, rhs);
            }

            bool operator==(const EdgeKey& other) const
            {
                return a == other.a && b == other.b;
            }
        };

        struct EdgeKeyHash
        {
            size_t operator()(const EdgeKey& key) const
            {
                const size_t h1 = std::hash<uint32_t>{}(key.a);
                const size_t h2 = std::hash<uint32_t>{}(key.b);
                return h1 ^ (h2 << 1);
            }
        };

        struct ObjCorner
        {
            Vertex vertex{};
            bool hasTexCoord = false;
            uint32_t triangleIndex = 0;
            uint32_t localCornerIndex = 0;
            uint32_t positionNodeIndex = 0;
        };

        struct ObjTriangle
        {
            std::array<uint32_t, 3> cornerIndices{};
            std::array<uint32_t, 3> positionNodeIndices{};
            glm::vec3 faceNormal = glm::vec3(0.0f);
            bool isDegenerate = false;
        };

        struct TriangleEdgeRef
        {
            uint32_t triangleIndex = 0;
            std::array<uint32_t, 2> localCornerIndices{};
        };

        struct FinalTriangle
        {
            std::array<uint32_t, 3> vertexIndices{};
        };

        struct MeshletBuildState
        {
            std::vector<uint32_t> triangles;
            std::unordered_set<uint32_t> triangleSet;
            std::unordered_set<uint32_t> uniqueVertices;
        };

        glm::vec3 ReadVec3(const std::vector<float>& data, int index)
        {
            const size_t baseIndex = static_cast<size_t>(index) * 3;
            return glm::vec3(data[baseIndex], data[baseIndex + 1], data[baseIndex + 2]);
        }

        glm::vec2 ReadVec2(const std::vector<float>& data, int index)
        {
            const size_t baseIndex = static_cast<size_t>(index) * 2;
            return glm::vec2(data[baseIndex], data[baseIndex + 1]);
        }

        bool TryGetSharedEdgeCornerIndex(const ObjTriangle& triangle,
                                         uint32_t positionNodeIndex,
                                         uint32_t& outLocalCorner)
        {
            for (uint32_t localCorner = 0; localCorner < 3; ++localCorner)
            {
                if (triangle.positionNodeIndices[localCorner] == positionNodeIndex)
                {
                    outLocalCorner = localCorner;
                    return true;
                }
            }

            return false;
        }

        bool AreSharedEdgeTexCoordsCompatible(const ObjTriangle& lhsTriangle,
                                              const ObjTriangle& rhsTriangle,
                                              const std::vector<ObjCorner>& corners,
                                              const EdgeKey& edge)
        {
            uint32_t lhsCornerA = 0;
            uint32_t lhsCornerB = 0;
            uint32_t rhsCornerA = 0;
            uint32_t rhsCornerB = 0;
            if (!TryGetSharedEdgeCornerIndex(lhsTriangle, edge.a, lhsCornerA)
                || !TryGetSharedEdgeCornerIndex(lhsTriangle, edge.b, lhsCornerB)
                || !TryGetSharedEdgeCornerIndex(rhsTriangle, edge.a, rhsCornerA)
                || !TryGetSharedEdgeCornerIndex(rhsTriangle, edge.b, rhsCornerB))
            {
                return false;
            }

            // 4 corners of the edge
            const ObjCorner& lhsA = corners[lhsTriangle.cornerIndices[lhsCornerA]];
            const ObjCorner& lhsB = corners[lhsTriangle.cornerIndices[lhsCornerB]];
            const ObjCorner& rhsA = corners[rhsTriangle.cornerIndices[rhsCornerA]];
            const ObjCorner& rhsB = corners[rhsTriangle.cornerIndices[rhsCornerB]];

            if (!lhsA.hasTexCoord || !lhsB.hasTexCoord || !rhsA.hasTexCoord || !rhsB.hasTexCoord)
            {
                return false;
            }

            return lhsA.vertex.texCoord.x == rhsA.vertex.texCoord.x
                   && lhsA.vertex.texCoord.y == rhsA.vertex.texCoord.y
                   && lhsB.vertex.texCoord.x == rhsB.vertex.texCoord.x
                   && lhsB.vertex.texCoord.y == rhsB.vertex.texCoord.y;
        }

        bool ShouldKeepAdjacency(const ObjTriangle& lhsTriangle,
                                 const ObjTriangle& rhsTriangle,
                                 const std::vector<ObjCorner>& corners,
                                 const EdgeKey& edge)
        {
            if (!AreSharedEdgeTexCoordsCompatible(lhsTriangle, rhsTriangle, corners, edge))
            {
                return false;
            }

            if (lhsTriangle.isDegenerate || rhsTriangle.isDegenerate)
            {
                return false;
            }

            return glm::dot(lhsTriangle.faceNormal, rhsTriangle.faceNormal) >= kHardEdgeDotThreshold;
        }

        uint32_t CountSharedVertices(const FinalTriangle& lhs, const FinalTriangle& rhs)
        {
            uint32_t sharedVertices = 0;
            for (const uint32_t lhsVertex : lhs.vertexIndices)
            {
                for (const uint32_t rhsVertex : rhs.vertexIndices)
                {
                    if (lhsVertex == rhsVertex)
                    {
                        ++sharedVertices;
                        break;
                    }
                }
            }

            return sharedVertices;
        }

        std::vector<std::vector<uint32_t>> BuildFinalTriangleAdjacency(const std::vector<FinalTriangle>& triangles)
        {
            std::unordered_map<EdgeKey, std::vector<uint32_t>, EdgeKeyHash> finalEdgeMap;
            finalEdgeMap.reserve(triangles.size() * 3);

            for (uint32_t triangleIndex = 0; triangleIndex < static_cast<uint32_t>(triangles.size()); ++triangleIndex)
            {
                const auto& triangle = triangles[triangleIndex];
                for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                {
                    const uint32_t nextEdgeIndex = (edgeIndex + 1) % 3;
                    finalEdgeMap[EdgeKey(triangle.vertexIndices[edgeIndex], triangle.vertexIndices[nextEdgeIndex])]
                        .push_back(triangleIndex);
                }
            }

            std::vector<std::vector<uint32_t>> adjacency(triangles.size());
            for (const auto& [edge, owners] : finalEdgeMap)
            {
                (void)edge;
                if (owners.size() < 2)
                {
                    continue;
                }

                for (size_t i = 0; i < owners.size(); ++i)
                {
                    for (size_t j = i + 1; j < owners.size(); ++j)
                    {
                        adjacency[owners[i]].push_back(owners[j]);
                        adjacency[owners[j]].push_back(owners[i]);
                    }
                }
            }

            for (auto& neighbors : adjacency)
            {
                std::ranges::sort(neighbors);
                neighbors.erase(std::ranges::unique(neighbors).begin(), neighbors.end());
            }

            return adjacency;
        }

        bool CanAddTriangleToMeshlet(const MeshletBuildState& meshlet,
                                     const FinalTriangle& triangle,
                                     const uint32_t maxMeshletVertices,
                                     const uint32_t maxMeshletIndices)
        {
            if ((meshlet.triangles.size() + 1) * 3 > maxMeshletIndices)
            {
                return false;
            }

            size_t uniqueVertexCount = meshlet.uniqueVertices.size();
            for (const uint32_t vertexIndex : triangle.vertexIndices)
            {
                if (!meshlet.uniqueVertices.contains(vertexIndex))
                {
                    ++uniqueVertexCount;
                }
            }

            return uniqueVertexCount <= maxMeshletVertices;
        }

        std::vector<MeshletBuildState> BuildMeshlets(const std::vector<FinalTriangle>& finalTriangles,
                                                     const std::vector<std::vector<uint32_t>>& triangleAdjacency,
                                                     const uint32_t maxMeshletVertices,
                                                     const uint32_t maxMeshletIndices)
        {
            std::vector<MeshletBuildState> meshlets;
            std::vector<uint8_t> assignedTriangles(finalTriangles.size(), 0);

            for (uint32_t seedTriangle = 0; seedTriangle < static_cast<uint32_t>(finalTriangles.size()); ++seedTriangle)
            {
                if (assignedTriangles[seedTriangle] != 0)
                {
                    continue;
                }

                MeshletBuildState meshlet;
                meshlet.triangles.push_back(seedTriangle);
                meshlet.triangleSet.insert(seedTriangle);
                for (const uint32_t vertexIndex : finalTriangles[seedTriangle].vertexIndices)
                {
                    meshlet.uniqueVertices.insert(vertexIndex);
                }
                assignedTriangles[seedTriangle] = 1;

                while (true)
                {
                    int32_t bestCandidate = -1;
                    uint32_t bestSharedEdges = 0;
                    uint32_t bestAddedVertices = std::numeric_limits<uint32_t>::max();

                    for (const uint32_t triangleIndex : meshlet.triangles)
                    {
                        for (const uint32_t candidateTriangle : triangleAdjacency[triangleIndex])
                        {
                            if (assignedTriangles[candidateTriangle] != 0
                                || meshlet.triangleSet.contains(candidateTriangle))
                            {
                                continue;
                            }

                            const auto& candidate = finalTriangles[candidateTriangle];
                            if (!CanAddTriangleToMeshlet(meshlet, candidate, maxMeshletVertices, maxMeshletIndices))
                            {
                                continue;
                            }

                            const uint32_t sharedVertices =
                                CountSharedVertices(finalTriangles[triangleIndex], candidate);
                            const uint32_t sharedEdges = sharedVertices >= 2 ? 1u : 0u;

                            uint32_t addedVertices = 0;
                            for (const uint32_t vertexIndex : candidate.vertexIndices)
                            {
                                if (!meshlet.uniqueVertices.contains(vertexIndex))
                                {
                                    ++addedVertices;
                                }
                            }

                            if (bestCandidate < 0 || sharedEdges > bestSharedEdges
                                || (sharedEdges == bestSharedEdges && addedVertices < bestAddedVertices)
                                || (sharedEdges == bestSharedEdges && addedVertices == bestAddedVertices
                                    && candidateTriangle < static_cast<uint32_t>(bestCandidate)))
                            {
                                bestCandidate = static_cast<int32_t>(candidateTriangle);
                                bestSharedEdges = sharedEdges;
                                bestAddedVertices = addedVertices;
                            }
                        }
                    }

                    if (bestCandidate < 0)
                    {
                        break;
                    }

                    const uint32_t triangleIndex = static_cast<uint32_t>(bestCandidate);
                    meshlet.triangles.push_back(triangleIndex);
                    meshlet.triangleSet.insert(triangleIndex);
                    for (const uint32_t vertexIndex : finalTriangles[triangleIndex].vertexIndices)
                    {
                        meshlet.uniqueVertices.insert(vertexIndex);
                    }
                    assignedTriangles[triangleIndex] = 1;
                }

                meshlets.push_back(std::move(meshlet));
            }

            return meshlets;
        }
    }

    YAML::Node MeshAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(uuid);
        rootNode["GenerateMeshlets"] = generateMeshlets;
        return rootNode;
    }

    MeshAssetMeta MeshAssetMeta::Deserialize(const YAML::Node& node)
    {
        MeshAssetMeta meta;
        meta.uuid = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        meta.generateMeshlets = node["GenerateMeshlets"] ? node["GenerateMeshlets"].as<bool>() : false;
        return meta;
    }

    void MeshAsset::Recreate()
    {
        if (m_IsLoaded)
        {
            m_Renderer->GetDevice()->WaitIdle();
            Unload();
            Load();
        }
    }

    void MeshAsset::LoadMeshFromFile()
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, m_FilePath.string().c_str()))
        {
            HZ_CORE_ERROR("Failed to load OBJ file '{}': {}", m_FilePath.string(), err);
            return;
        }

        if (!warn.empty())
        {
            HZ_CORE_WARN("Warnings while loading OBJ file '{}': {}", m_FilePath.string(), warn);
        }

        std::unordered_map<PositionKey, uint32_t, PositionKeyHash> positionNodeMap;
        std::vector<ObjCorner> corners;
        std::vector<ObjTriangle> triangles;
        std::unordered_map<EdgeKey, std::vector<TriangleEdgeRef>, EdgeKeyHash> edgeMap;
        std::vector<std::vector<uint32_t>> cornersPerPositionNode;

        // iterate over the obj shapes the first time
        for (const auto& shape : shapes)
        {
            // current index start
            size_t indexOffset = 0;
            for (const unsigned char vertexCount : shape.mesh.num_face_vertices)
            {
                if (vertexCount != 3)
                {
                    HZ_CORE_WARN(
                        "Skipping non-triangle face with {} vertices while loading OBJ '{}'.",
                        static_cast<uint32_t>(vertexCount),
                        m_FilePath.string());
                    indexOffset += vertexCount;
                    continue;
                }

                ObjTriangle triangle;
                const uint32_t triangleIndex = static_cast<uint32_t>(triangles.size());

                // for each triangle corner
                for (uint32_t localCorner = 0; localCorner < 3; ++localCorner)
                {
                    const tinyobj::index_t index = shape.mesh.indices[indexOffset + localCorner];

                    ObjCorner corner;
                    corner.vertex.position = ReadVec3(attrib.vertices, index.vertex_index);
                    corner.hasTexCoord = index.texcoord_index >= 0;
                    if (corner.hasTexCoord)
                    {
                        corner.vertex.texCoord = ReadVec2(attrib.texcoords, index.texcoord_index);
                    }
                    if (index.normal_index >= 0)
                    {
                        corner.vertex.normal = ReadVec3(attrib.normals, index.normal_index);
                    }
                    corner.vertex.tangent = glm::vec3(0.0f);
                    corner.triangleIndex = triangleIndex;
                    corner.localCornerIndex = localCorner;

                    // dedup the position
                    const PositionKey key{corner.vertex.position};
                    auto [it, inserted] = positionNodeMap.try_emplace(
                        key,
                        static_cast<uint32_t>(positionNodeMap.size()));
                    if (inserted)
                    {
                        cornersPerPositionNode.emplace_back();
                    }

                    corner.positionNodeIndex = it->second;
                    const uint32_t cornerIndex = static_cast<uint32_t>(corners.size());
                    corners.push_back(corner);
                    cornersPerPositionNode[corner.positionNodeIndex].push_back(cornerIndex);

                    triangle.cornerIndices[localCorner] = cornerIndex;
                    triangle.positionNodeIndices[localCorner] = corner.positionNodeIndex;
                }

                // face normal
                const glm::vec3& p0 = corners[triangle.cornerIndices[0]].vertex.position;
                const glm::vec3& p1 = corners[triangle.cornerIndices[1]].vertex.position;
                const glm::vec3& p2 = corners[triangle.cornerIndices[2]].vertex.position;
                const glm::vec3 cross = glm::cross(p1 - p0, p2 - p0);
                const float crossLength = glm::length(cross);
                if (crossLength <= kDegenerateTriangleEpsilon)
                {
                    triangle.isDegenerate = true;
                }
                else
                {
                    triangle.faceNormal = cross / crossLength;
                }

                triangles.push_back(triangle);

                // 3 edges for the triangle
                for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                {
                    const uint32_t nextEdgeIndex = (edgeIndex + 1) % 3;
                    EdgeKey edge(
                        triangle.positionNodeIndices[edgeIndex],
                        triangle.positionNodeIndices[nextEdgeIndex]);
                    // tracking all triangles an edge belongs to
                    edgeMap[edge].push_back(
                        TriangleEdgeRef{triangleIndex, {edgeIndex, nextEdgeIndex}});
                }

                indexOffset += vertexCount;
            }
        }

        // adjacency maps
        std::vector<std::vector<uint32_t>> cornerAdjacency(corners.size());
        std::vector<std::vector<uint32_t>> triangleAdjacency(triangles.size());
        for (const auto& [edge, triangleRefs] : edgeMap)
        {
            if (triangleRefs.size() < 2)
            {
                continue;
            }

            // all triangles sharing the edge
            for (size_t i = 0; i < triangleRefs.size(); ++i)
            {
                for (size_t j = i + 1; j < triangleRefs.size(); ++j)
                {
                    const ObjTriangle& lhsTriangle = triangles[triangleRefs[i].triangleIndex];
                    const ObjTriangle& rhsTriangle = triangles[triangleRefs[j].triangleIndex];
                    if (!ShouldKeepAdjacency(lhsTriangle, rhsTriangle, corners, edge))
                    {
                        continue;
                    }

                    // add adjacency between the triangles
                    triangleAdjacency[triangleRefs[i].triangleIndex].push_back(triangleRefs[j].triangleIndex);
                    triangleAdjacency[triangleRefs[j].triangleIndex].push_back(triangleRefs[i].triangleIndex);

                    for (const uint32_t sharedPositionNode : {edge.a, edge.b})
                    {
                        uint32_t lhsCorner = 0;
                        uint32_t rhsCorner = 0;
                        if (!TryGetSharedEdgeCornerIndex(lhsTriangle, sharedPositionNode, lhsCorner)
                            || !TryGetSharedEdgeCornerIndex(rhsTriangle, sharedPositionNode, rhsCorner))
                        {
                            continue;
                        }

                        const uint32_t lhsCornerIndex = lhsTriangle.cornerIndices[lhsCorner];
                        const uint32_t rhsCornerIndex = rhsTriangle.cornerIndices[rhsCorner];
                        cornerAdjacency[lhsCornerIndex].push_back(rhsCornerIndex);
                        cornerAdjacency[rhsCornerIndex].push_back(lhsCornerIndex);
                    }
                }
            }
        }

        // there are duplications from the previous step so dedup
        for (auto& neighbors : triangleAdjacency)
        {
            std::ranges::sort(neighbors);
            neighbors.erase(std::ranges::unique(neighbors).begin(), neighbors.end());
        }

        std::vector<Vertex> finalVertices;
        std::vector<uint32_t> finalIndices;
        finalIndices.reserve(triangles.size() * 3);
        std::vector<uint32_t> finalVertexIndexPerCorner(corners.size(), std::numeric_limits<uint32_t>::max());
        std::vector<uint8_t> visitedCorners(corners.size(), 0); // byte map for duplicated corners

        // aggregate corners
        for (const auto& cornerGroup : cornersPerPositionNode)
        {
            for (const uint32_t seedCornerIndex : cornerGroup)
            {
                if (visitedCorners[seedCornerIndex] != 0)
                {
                    continue;
                }

                const uint32_t finalVertexIndex = static_cast<uint32_t>(finalVertices.size());
                finalVertices.push_back(corners[seedCornerIndex].vertex);

                std::queue<uint32_t> pendingCorners;
                pendingCorners.push(seedCornerIndex);
                visitedCorners[seedCornerIndex] = 1;

                // set for all adjacent corners
                while (!pendingCorners.empty())
                {
                    const uint32_t cornerIndex = pendingCorners.front();
                    pendingCorners.pop();
                    finalVertexIndexPerCorner[cornerIndex] = finalVertexIndex;

                    for (const uint32_t adjacentCornerIndex : cornerAdjacency[cornerIndex])
                    {
                        if (visitedCorners[adjacentCornerIndex] != 0)
                        {
                            continue;
                        }

                        visitedCorners[adjacentCornerIndex] = 1;
                        pendingCorners.push(adjacentCornerIndex);
                    }
                }
            }
        }

        std::vector<FinalTriangle> finalTriangles;
        finalTriangles.reserve(triangles.size());
        for (const ObjTriangle& triangle : triangles)
        {
            FinalTriangle finalTriangle;
            for (uint32_t localCorner = 0; localCorner < 3; ++localCorner)
            {
                const uint32_t finalVertexIndex = finalVertexIndexPerCorner[triangle.cornerIndices[localCorner]];
                finalTriangle.vertexIndices[localCorner] = finalVertexIndex;
                finalIndices.push_back(finalVertexIndex);
            }

            finalTriangles.push_back(finalTriangle);
        }

        m_Vertices = std::move(finalVertices);
        m_Indices = std::move(finalIndices);

        if (!m_Meta.generateMeshlets)
        {
            return;
        }

        auto finalTriangleAdjacency = BuildFinalTriangleAdjacency(finalTriangles);
        auto meshletBuildStates =
            BuildMeshlets(finalTriangles, finalTriangleAdjacency, kMaxMeshletVertices, kMaxMeshletIndices);;
        if (meshletBuildStates.empty())
        {
            return;
        }

        std::vector<Vertex> meshletVertices;
        std::vector<uint32_t> meshletIndices;
        std::vector<MeshletInfo> meshlets;

        for (const auto& meshletBuildState : meshletBuildStates)
        {
            std::unordered_map<uint32_t, uint32_t> vertexRemap;
            vertexRemap.reserve(meshletBuildState.uniqueVertices.size());

            const uint32_t indexStart = static_cast<uint32_t>(meshletIndices.size());
            const uint32_t vertexStart = static_cast<uint32_t>(meshletVertices.size());
            for (const uint32_t triangleIndex : meshletBuildState.triangles)
            {
                const auto& triangle = finalTriangles[triangleIndex];
                for (const uint32_t sourceVertexIndex : triangle.vertexIndices)
                {
                    auto [it, inserted] = vertexRemap.try_emplace(sourceVertexIndex);
                    if (inserted)
                    {
                        it->second = static_cast<uint32_t>(vertexRemap.size() - 1);
                        meshletVertices.push_back(m_Vertices[sourceVertexIndex]);
                    }

                    meshletIndices.push_back(it->second);
                }
            }

            meshlets.push_back({indexStart,
                                static_cast<uint32_t>(meshletIndices.size()) - indexStart,
                                vertexStart,
                                static_cast<uint32_t>(meshletVertices.size()) - vertexStart});
        }

        m_Vertices = std::move(meshletVertices);
        m_Indices = std::move(meshletIndices);
        m_Meshlets = std::move(meshlets);
    }

    MeshAsset::MeshAsset(MeshAsset&& other) noexcept
    {
        m_UUID = other.m_UUID;
        m_Renderer = other.m_Renderer;
        m_FilePath = std::move(other.m_FilePath);
        m_Meta = other.m_Meta;
        m_Vertices = std::move(other.m_Vertices);
        m_Indices = std::move(other.m_Indices);
        m_Meshlets = std::move(other.m_Meshlets);
    }

    MeshAsset& MeshAsset::operator=(MeshAsset&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        m_UUID = other.m_UUID;
        m_Renderer = other.m_Renderer;
        m_FilePath = std::move(other.m_FilePath);
        m_Meta = other.m_Meta;
        m_Vertices = std::move(other.m_Vertices);
        m_Indices = std::move(other.m_Indices);
        m_Meshlets = std::move(other.m_Meshlets);
        return *this;
    }

    void MeshAsset::Load()
    {
        if (m_IsLoaded)
        {
            return;
        }

        if (m_Meta.generateMeshlets)
        {
            auto mesh = std::make_unique<Mesh>(m_UUID, m_Vertices, m_Indices, m_Meshlets);
            m_Mesh = m_Renderer->AddMesh(std::move(mesh));
        }
        else
        {
            auto mesh = std::make_unique<Mesh>(m_UUID, m_Vertices, m_Indices);
            m_Mesh = m_Renderer->AddMesh(std::move(mesh));
        }
    }

    void MeshAsset::Unload() {}

    MeshAsset::~MeshAsset() {}
} // Hazel