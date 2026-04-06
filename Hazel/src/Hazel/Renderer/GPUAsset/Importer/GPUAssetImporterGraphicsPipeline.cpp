//
// Created by helmholtz on 2026/4/5.
//

#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Asset/MeshAsset.h"
#include "Hazel/Core/UUID.h"
#include "GPUAssetImporter.h"
#include "GPUAssetImporterInternal.h"
#include "Hazel/Renderer/GPUAsset/GPUShaderAsset.h"

namespace Hazel
{
    RHIGraphicsPipeline* CreateGraphicsPipeline(UUID material,
                                                const std::vector<RHIFormat>& colorAttachmentFormats,
                                                RHIFormat depthStencilFormat,
                                                Renderer* renderer)
    {
        auto materialResult = renderer->ResolveGPUAssetBlocked(material, AssetType::Material);
        if (!materialResult.asset)
        {
            return nullptr;
        }

        auto shaderResult = renderer->ResolveGPUAssetBlocked(
            static_cast<CachedMaterial*>(materialResult.asset)->GetShader(),
            AssetType::Shader);

        if (!shaderResult.asset)
        {
            return nullptr;
        }

        auto* cachedMaterial = static_cast<CachedMaterial*>(materialResult.asset);
        auto* shader = static_cast<GPUShaderAsset*>(shaderResult.asset);

        RHIGraphicsPipelineDesc pipelineDesc{};

        auto reflection = shader->GetReflection();

        RHIResourceSignatureDesc resourceSignatureDesc{};

        pipelineDesc.resourceSignature = renderer->GetMaterialShaderRegistry()->GetShaderResourceSignature(
            cachedMaterial->GetShader(),
            cachedMaterial->GetShaderSourceVersion());

        if (!pipelineDesc.resourceSignature || !pipelineDesc.resourceSignature->IsValid())
        {
            return nullptr;
        }

        pipelineDesc.vertexShader = shader->GetVertexShader();
        pipelineDesc.fragmentShader = shader->GetFragmentShader();

        pipelineDesc.vertexBindings = {
            {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = RHIVertexInputRate::Vertex
            }
        };

        pipelineDesc.vertexAttributes = {
            {
                .location = 0,
                .binding = 0,
                .format = RHIFormat::RGB32SFloat,
                .offset = offsetof(Vertex, position)
            },
            {
                .location = 1,
                .binding = 0,
                .format = RHIFormat::RG32SFloat,
                .offset = offsetof(Vertex, texCoord)
            },
            {
                .location = 2,
                .binding = 0,
                .format = RHIFormat::RGB32SFloat,
                .offset = offsetof(Vertex, normal)
            },
            {
                .location = 3,
                .binding = 0,
                .format = RHIFormat::RGB32SFloat,
                .offset = offsetof(Vertex, tangent)
            }
        };

        pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;
        pipelineDesc.polygonMode = cachedMaterial->GetPipelineState().polygonMode;
        pipelineDesc.cullMode = cachedMaterial->GetPipelineState().cullMode;
        pipelineDesc.depthClampEnable = cachedMaterial->GetPipelineState().depthClampEnable;
        pipelineDesc.depthBiasEnable = cachedMaterial->GetPipelineState().depthBiasEnable;
        pipelineDesc.depthTestEnable = cachedMaterial->GetPipelineState().depthTestEnable;
        pipelineDesc.depthWriteEnable = cachedMaterial->GetPipelineState().depthWriteEnable;
        pipelineDesc.depthCompareOp = cachedMaterial->GetPipelineState().depthCompareOp;
        pipelineDesc.stencilTestEnable = cachedMaterial->GetPipelineState().stencilTestEnable;
        pipelineDesc.colorBlendAttachments = cachedMaterial->GetPipelineState().colorBlendAttachments;

        pipelineDesc.colorAttachmentFormats = colorAttachmentFormats;
        pipelineDesc.depthStencilFormat = depthStencilFormat;

        auto pipeline = renderer->GetDevice()->CreateGraphicsPipeline(pipelineDesc);
        if (!pipeline || !pipeline->IsValid())
        {
            return nullptr;
        }

        return pipeline;
    }
}