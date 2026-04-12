//
// Created by helmholtz on 2026/4/2.
//

#include "AssetUtils.h"

#include <fstream>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    AssetType InferAssetTypeFromPath(const std::filesystem::path& path)
    {
        const auto extension = path.extension().string();

        static const std::unordered_set<std::string> imageExtensions = {
            ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".hdr"};

        if (imageExtensions.contains(extension)) { return AssetType::Texture; }

        if (extension == ".comp") { return AssetType::ComputeShader; }

        if (extension == ".obj") { return AssetType::Mesh; }

        if (extension == ".shader") { return AssetType::Shader; }

        if (extension == ".meta")
        {
            const auto stemFilePath = path.stem();
            const auto stemExtension = stemFilePath.extension().string();

            if (stemExtension == ".rt") { return AssetType::RenderTexture; }

            if (stemExtension == ".sampler") { return AssetType::Sampler; }

            if (stemExtension == ".mat") { return AssetType::Material; }
        }

        return AssetType::Unknown;
    }

    std::filesystem::path GetMetaPathFromAssetPath(const std::filesystem::path& assetPath)
    {
        if (assetPath.extension() == ".meta") { return assetPath; }
        auto metaPath = assetPath;
        metaPath += ".meta";
        return metaPath;
    }

    UUID GetUUIDFromMetaFile(const std::filesystem::path& metaPath)
    {
        if (!(std::filesystem::is_regular_file(metaPath) && std::filesystem::exists(metaPath))) { return UUID(-1); }

        YAML::Node metaNode = YAML::LoadFile(metaPath.string());
        if (!metaNode["UUID"]) { return UUID(-1); }

        return UUID(metaNode["UUID"].as<uint64_t>());
    }
} // namespace Hazel