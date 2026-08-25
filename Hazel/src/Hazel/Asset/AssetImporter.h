// Declares CPU asset importer entry points.
// Created: 2026-04-02.

#pragma once
#include <filesystem>

namespace Aster
{
    class AssetManager;
    class ComputeShaderAsset;
    class MeshAsset;
    class MaterialAsset;
    struct AssetRegistryTerm;
    class RenderTextureAsset;
    class ShaderAsset;
    class TextureAsset;
    class SamplerAsset;

    class AssetImporter
    {
      public:
        static std::unique_ptr<ComputeShaderAsset> ImportComputeShader(AssetRegistryTerm* registryTerm);
        static std::unique_ptr<MeshAsset> ImportMesh(AssetRegistryTerm* registryTerm);
        static std::unique_ptr<MaterialAsset> ImportMaterial(AssetManager* assetManager,
                                                             AssetRegistryTerm* registryTerm);
        static std::unique_ptr<RenderTextureAsset> ImportRenderTexture(AssetRegistryTerm* registryTerm);
        static std::unique_ptr<ShaderAsset> ImportShader(AssetRegistryTerm* registryTerm);
        static std::unique_ptr<TextureAsset> ImportTexture(AssetRegistryTerm* registryTerm);
        static std::unique_ptr<SamplerAsset> ImportSampler(AssetRegistryTerm* registryTerm);
    };
} // namespace Aster
