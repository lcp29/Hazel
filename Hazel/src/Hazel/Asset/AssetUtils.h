//
// Created by helmholtz on 2026/4/2.
//

#pragma once
#include "Asset.h"

#include <yaml-cpp/yaml.h>

namespace Hazel
{
    AssetType InferAssetTypeFromPath(const std::filesystem::path& path);
    std::filesystem::path GetMetaPathFromAssetPath(const std::filesystem::path& assetPath);
    UUID GetUUIDFromMetaFile(const std::filesystem::path& metaPath);

    template <typename T> void WriteMetaToFile(const T& meta, const std::filesystem::path& metaPath)
    {
        YAML::Node node = meta.Serialize();
        std::ofstream output(metaPath);
        if (output) { output << node; }
        output.close();
    }

    template <typename T> void WriteMetaToFile(T* asset)
    {
        if (!asset) { return; }
        WriteMetaToFile(asset->GetMeta(), GetMetaPathFromAssetPath(asset->GetAbsoluteFilePath()));
    }

    template <typename T> T ReadMetaFromFile(const std::filesystem::path& metaPath)
    {
        if (!(std::filesystem::is_regular_file(metaPath) && std::filesystem::exists(metaPath)))
        {
            return T::CreateDefault();
        }

        YAML::Node metaNode = YAML::LoadFile(metaPath.string());
        return T::Deserialize(metaNode);
    }
} // namespace Hazel