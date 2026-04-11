//
// Created by helmholtz on 2026/4/3.
//

#pragma once

#include "MeshAsset.h"

#include <utility>

namespace Hazel
{
    inline constexpr auto MeshImportHardEdgeDegreesString = "asset.MeshImport.HardEdgeDegrees";
    inline constexpr auto MeshImportDegenerateTriangleEpsilonString = "asset.MeshImport.DegenerateTriangleEpsilon";
    inline constexpr float DefaultMeshImportHardEdgeDegrees = 45.0f;
    inline constexpr float DefaultMeshImportDegenerateTriangleEpsilon = 1e-8f;

    std::pair<glm::vec3, float> ComputeRitterBoundingSphere(const std::vector<Vertex>& vertices);
    bool ImportMeshAssetData(const std::filesystem::path& filePath, bool generateMeshlets, MeshAssetData& outData);
}