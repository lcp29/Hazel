//
// Created by helmholtz on 2026/4/3.
//

#pragma once

#include "MeshAsset.h"

namespace Hazel
{
    bool ImportMeshAssetData(const std::filesystem::path& filePath, bool generateMeshlets, MeshAssetData& outData);
}
