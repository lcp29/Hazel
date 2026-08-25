// Declares GPU asset importer entry points.
// Created: 2026-04-03.

#pragma once
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

#include <memory>

namespace Hazel
{
    class Renderer;
}

namespace Aster
{
    class GPUAsset;
    class Asset;
    class ComputeShaderAsset;
    class GPUComputeShaderAsset;
    class RenderTextureAsset;
    class GPURenderTextureAsset;
    class SamplerAsset;
    class GPUSamplerAsset;
    class ShaderAsset;
    class GPUShaderAsset;
    class TextureAsset;
    class GPUTextureAsset;
    class CachedMaterial;
    class MaterialAsset;
    class MeshAsset;
    class GPUMeshAsset;

    std::unique_ptr<GPUComputeShaderAsset> ImportGPUComputeShaderAsset(Hazel::Renderer* renderer,
                                                                       const ComputeShaderAsset* asset);
    std::unique_ptr<GPURenderTextureAsset> ImportGPURenderTextureAsset(Hazel::Renderer* renderer,
                                                                       const RenderTextureAsset* asset);
    std::unique_ptr<GPUSamplerAsset> ImportGPUSamplerAsset(Hazel::Renderer* renderer, const SamplerAsset* asset);
    std::unique_ptr<GPUShaderAsset> ImportGPUShaderAsset(Hazel::Renderer* renderer, const ShaderAsset* asset);
    std::unique_ptr<GPUTextureAsset> ImportGPUTextureAsset(Hazel::Renderer* renderer, const TextureAsset* asset);
    std::unique_ptr<CachedMaterial> ImportCachedMaterial(Hazel::Renderer* renderer, const MaterialAsset* asset);
    std::unique_ptr<GPUMeshAsset> ImportGPUMeshAsset(Hazel::Renderer* renderer, const MeshAsset* asset);

    RHIGraphicsPipeline* CreateGraphicsPipeline(Hazel::UUID material,
                                                const std::vector<RHIFormat>& colorAttachmentFormats,
                                                const std::vector<RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                                                RHIFormat depthStencilFormat,
                                                Hazel::Renderer* renderer);
} // namespace Aster
