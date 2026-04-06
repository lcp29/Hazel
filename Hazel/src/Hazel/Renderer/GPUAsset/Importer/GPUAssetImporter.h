//
// Created by helmholtz on 2026/4/3.
//

#pragma once
#include "Hazel/RHI/RHI.h"

#include <memory>

namespace Hazel
{
    class GPUAsset;
    class Asset;
    class ComputeShaderAsset;
    class GPUComputeShaderAsset;
    class Renderer;
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

    std::unique_ptr<GPUComputeShaderAsset> ImportGPUComputeShaderAsset(Renderer* renderer,
                                                                       const ComputeShaderAsset* asset);
    std::unique_ptr<GPURenderTextureAsset> ImportGPURenderTextureAsset(Renderer* renderer,
                                                                       const RenderTextureAsset* asset);
    std::unique_ptr<GPUSamplerAsset> ImportGPUSamplerAsset(Renderer* renderer, const SamplerAsset* asset);
    std::unique_ptr<GPUShaderAsset> ImportGPUShaderAsset(Renderer* renderer, const ShaderAsset* asset);
    std::unique_ptr<GPUTextureAsset> ImportGPUTextureAsset(Renderer* renderer, const TextureAsset* asset);
    std::unique_ptr<CachedMaterial> ImportCachedMaterial(Renderer* renderer, const MaterialAsset* asset);

    RHIGraphicsPipeline* CreateGraphicsPipeline(UUID material,
                                                const std::vector<RHIFormat>& colorAttachmentFormats,
                                                RHIFormat depthStencilFormat,
                                                Renderer* renderer);
} // Hazel