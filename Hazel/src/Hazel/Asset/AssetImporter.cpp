//
// Created by helmholtz on 2026/4/2.
//

#include "AssetImporter.h"
#include "AssetManager.h"
#include "AssetUtils.h"
#include "ComputeShaderAsset.h"
#include "MaterialAsset.h"
#include "MeshAsset.h"
#include "MeshImportUtils.h"
#include "RenderTextureAsset.h"
#include "ShaderAsset.h"
#include "TextureAsset.h"
#include "Hazel/Renderer/ShaderCommon.h"

#include <filesystem>
#include <ranges>
#include <stb_image.h>

namespace Hazel
{
    std::unique_ptr<ComputeShaderAsset> AssetImporter::ImportComputeShader(AssetRegistryTerm* registryTerm)
    {
        auto meta = ReadMetaFromFile<ComputeShaderAssetMeta>(GetMetaPathFromAssetPath(registryTerm->filePath));

        RHIShaderFileDesc shaderFileDesc{};
        shaderFileDesc.path = registryTerm->filePath;
        shaderFileDesc.entryPoint = "main";
        shaderFileDesc.stage = RHIShaderStageFlagBits::Compute;
        shaderFileDesc.debugName = registryTerm->filePath.filename().string() + " [CS]";
        shaderFileDesc.macroDefinitions.push_back({"COMPUTE_SHADER", ""});
        auto compileResult = CompileShaderFileToSPIRV(shaderFileDesc);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            return nullptr;
        }

        ComputeShaderAssetData data;
        data.binary.assign(compileResult.cbegin(), compileResult.cend());
        if (!ReflectShaderSPIRV(data.binary, data.reflection))
        {
            return nullptr;
        }

        return std::make_unique<ComputeShaderAsset>(registryTerm, meta, std::move(data));
    }

    std::unique_ptr<MeshAsset> AssetImporter::ImportMesh(AssetRegistryTerm* registryTerm)
    {
        auto meta = ReadMetaFromFile<MeshAssetMeta>(GetMetaPathFromAssetPath(registryTerm->filePath));

        MeshAssetData data;
        if (!ImportMeshAssetData(registryTerm->filePath, meta.GenerateMeshlets(), data))
        {
            return nullptr;
        }

        return std::make_unique<MeshAsset>(registryTerm, meta, std::move(data));
    }

    std::unique_ptr<MaterialAsset> AssetImporter::ImportMaterial(AssetManager* assetManager,
                                                                 AssetRegistryTerm* registryTerm)
    {
        auto meta = ReadMetaFromFile<MaterialAssetMeta>(GetMetaPathFromAssetPath(registryTerm->filePath));

        if (meta.GetShader() == UUID(-1))
        {
            meta.ClearProperties();
            return std::make_unique<MaterialAsset>(registryTerm, meta);
        }

        auto* shaderAsset = static_cast<ShaderAsset*>(assetManager->RequestAssetBlocked(meta.GetShader()));
        if (!shaderAsset)
        {
            return nullptr;
        }

        for (auto [i, metaProperty] : std::views::enumerate(meta.GetProperties()))
        {
            MaterialAssetProperty property;
            property.name = metaProperty.name;
            property.type = metaProperty.type;
            std::copy_n(metaProperty.data, 64, property.data);
            property.sampler = metaProperty.sampler;
            property.texture = metaProperty.texture;

            switch (metaProperty.type)
            {
                case MaterialAssetPropertyType::Sampler:
                    if (!assetManager->RequestAssetBlocked(metaProperty.sampler))
                    {
                        meta.SetPropertySampler(i, UUID(-1));
                    }
                    break;
                case MaterialAssetPropertyType::Texture:
                    if (!assetManager->RequestAssetBlocked(metaProperty.texture))
                    {
                        meta.SetPropertyTexture(i, UUID(-1));
                    }
                    break;
                case MaterialAssetPropertyType::SamplerWithTexture:
                    if (!assetManager->RequestAssetBlocked(metaProperty.sampler))
                    {
                        meta.SetPropertySampler(i, UUID(-1));
                    }
                    if (!assetManager->RequestAssetBlocked(metaProperty.texture))
                    {
                        meta.SetPropertyTexture(i, UUID(-1));
                    }
                    break;
                default:
                    break;
            }
        }

        meta.RefreshShader(assetManager);

        return std::make_unique<MaterialAsset>(registryTerm, meta);
    }

    std::unique_ptr<RenderTextureAsset> AssetImporter::ImportRenderTexture(AssetRegistryTerm* registryTerm)
    {
        auto meta = ReadMetaFromFile<RenderTextureAssetMeta>(GetMetaPathFromAssetPath(registryTerm->filePath));
        return std::make_unique<RenderTextureAsset>(registryTerm, meta);
    }

    std::unique_ptr<ShaderAsset> AssetImporter::ImportShader(AssetRegistryTerm* registryTerm)
    {
        auto meta = ReadMetaFromFile<ShaderAssetMeta>(GetMetaPathFromAssetPath(registryTerm->filePath));

        RHIShaderFileDesc vertexShaderFileDesc{};
        vertexShaderFileDesc.path = registryTerm->filePath;
        vertexShaderFileDesc.entryPoint = "main";
        vertexShaderFileDesc.stage = RHIShaderStageFlagBits::Vertex;
        vertexShaderFileDesc.debugName = registryTerm->filePath.filename().string() + " [VS]";
        vertexShaderFileDesc.macroDefinitions.push_back({"VERTEX_SHADER", ""});
        auto vertexCompileResult = CompileShaderFileToSPIRV(vertexShaderFileDesc);
        if (vertexCompileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            return nullptr;
        }

        RHIShaderFileDesc fragmentShaderFileDesc{};
        fragmentShaderFileDesc.path = registryTerm->filePath;
        fragmentShaderFileDesc.entryPoint = "main";
        fragmentShaderFileDesc.stage = RHIShaderStageFlagBits::Fragment;
        fragmentShaderFileDesc.debugName = registryTerm->filePath.filename().string() + " [FS]";
        fragmentShaderFileDesc.macroDefinitions.push_back({"FRAGMENT_SHADER", ""});
        auto fragmentCompileResult = CompileShaderFileToSPIRV(fragmentShaderFileDesc);
        if (fragmentCompileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            return nullptr;
        }

        ShaderAssetData data;
        data.vertexBinary.assign(vertexCompileResult.cbegin(), vertexCompileResult.cend());
        data.fragmentBinary.assign(fragmentCompileResult.cbegin(), fragmentCompileResult.cend());

        RHIShaderReflection vertexReflection;
        if (!ReflectShaderSPIRV(data.vertexBinary, vertexReflection))
        {
            return nullptr;
        }

        RHIShaderReflection fragmentReflection;
        if (!ReflectShaderSPIRV(data.fragmentBinary, fragmentReflection))
        {
            return nullptr;
        }

        data.reflection = MergeShaderReflections(vertexReflection, fragmentReflection);
        return std::make_unique<ShaderAsset>(registryTerm, meta, std::move(data));
    }

    std::unique_ptr<TextureAsset> AssetImporter::ImportTexture(AssetRegistryTerm* registryTerm)
    {
        const auto& filePath = registryTerm->filePath;
        const auto filePathString = filePath.string();

        auto meta = ReadMetaFromFile<TextureAssetMeta>(GetMetaPathFromAssetPath(filePath));

        int width = 0;
        int height = 0;
        int channels = 0;
        int bytesPerPixel = 0;
        void* pixels;
        bool isHDR = false;

        if (stbi_is_hdr(filePathString.c_str()) == 1)
        {
            pixels = stbi_loadf(filePathString.c_str(),
                                &width,
                                &height,
                                &channels,
                                STBI_rgb);
            bytesPerPixel = 3 * 4;
            isHDR = true;
        }
        else
        {
            pixels = stbi_load(filePathString.c_str(),
                               &width,
                               &height,
                               &channels,
                               STBI_rgb_alpha);
            bytesPerPixel = 4;
        }

        if (!pixels || width <= 0 || height <= 0)
        {
            if (pixels)
            {
                stbi_image_free(pixels);
            }
            return nullptr;
        }

        const size_t dataSize = width * height * bytesPerPixel;
        std::vector<uint8_t> imageData(dataSize);
        std::memcpy(imageData.data(), pixels, dataSize);
        stbi_image_free(pixels);

        TextureAssetData data;
        data.width = width;
        data.height = height;
        data.format = isHDR ? RHIFormat::RGB32SFloat : (meta.IsSRGB() ? RHIFormat::RGBA8SRGB : RHIFormat::RGBA8UNorm);
        data.rawImageData = std::move(imageData);

        auto texture = std::make_unique<TextureAsset>(registryTerm, meta, std::move(data));
        return texture;
    }

    std::unique_ptr<SamplerAsset> AssetImporter::ImportSampler(AssetRegistryTerm* registryTerm)
    {
        auto meta = ReadMetaFromFile<SamplerAssetMeta>(GetMetaPathFromAssetPath(registryTerm->filePath));
        return std::make_unique<SamplerAsset>(registryTerm, meta);
    }
} // Hazel