#include "Hazel/Asset/ComputeShaderAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUComputeShaderAsset.h"
#include "Hazel/Renderer/GPUAsset/Importer/GPUAssetImporter.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/ShaderCommon.h"

namespace Aster
{
    std::unique_ptr<GPUComputeShaderAsset> ImportGPUComputeShaderAsset(Hazel::Renderer* renderer,
                                                                       const ComputeShaderAsset* asset)
    {
        const auto& computeShaderData = asset->GetData();

        RHIShaderDesc computeShaderDesc{};
        computeShaderDesc.stage = RHIShaderStageFlagBits::Compute;
        computeShaderDesc.entryPoint = "main";
        computeShaderDesc.binary = computeShaderData.binary;

        auto* device = renderer->GetDevice();
        auto* computeShader = device->CreateShader(computeShaderDesc);

        std::vector<RHIResourceLayoutDesc> setData;
        AddReflectionToSetData(setData, computeShaderData.reflection, RHIShaderStageFlagBits::Compute);

        std::vector<RHIResourceLayout*> resourceLayouts;
        resourceLayouts.reserve(setData.size());

        for (uint32_t set = 0; set < setData.size(); set++)
        {
            std::ranges::sort(setData[set].bindings,
                              [](const RHIResourceBindingSlotDesc& lhs, const RHIResourceBindingSlotDesc& rhs) {
                                  return lhs.slot < rhs.slot;
                              });
            auto* layout = device->CreateResourceLayout(setData[set]);
            resourceLayouts.push_back(layout);
        }

        RHIResourceSignatureDesc signatureDesc{};
        signatureDesc.resourceLayouts = resourceLayouts;
        signatureDesc.pushConstantRanges = BuildComputePushConstantRanges(computeShaderData.reflection);
        auto* resourceSignature = device->CreateResourceSignature(signatureDesc);

        RHIComputePipelineDesc pipelineDesc{};
        pipelineDesc.computeShader = computeShader;
        pipelineDesc.resourceSignature = resourceSignature;
        auto* computePipeline = device->CreateComputePipeline(pipelineDesc);

        return std::make_unique<GPUComputeShaderAsset>(asset->GetUUID(),
                                                       asset->GetVersion(),
                                                       renderer,
                                                       computeShader,
                                                       std::move(resourceLayouts),
                                                       resourceSignature,
                                                       computePipeline,
                                                       renderer->GetCurrentFrameIndex());
    }
} // namespace Aster
