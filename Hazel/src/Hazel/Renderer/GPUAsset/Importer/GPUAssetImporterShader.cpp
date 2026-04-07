#include "Hazel/Renderer/GPUAsset/Importer/GPUAssetImporter.h"

#include "Hazel/Asset/ShaderAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUShaderAsset.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/ShaderCommon.h"

namespace Hazel
{
    std::unique_ptr<GPUShaderAsset> ImportGPUShaderAsset(Renderer* renderer, const ShaderAsset* asset)
    {
        const auto& shaderData = asset->GetData();

        RHIShaderDesc vertexShaderDesc{};
        vertexShaderDesc.stage = RHIShaderStageFlagBits::Vertex;
        vertexShaderDesc.entryPoint = "main";
        vertexShaderDesc.binary = shaderData.vertexBinary;

        RHIShaderDesc fragmentShaderDesc{};
        fragmentShaderDesc.stage = RHIShaderStageFlagBits::Fragment;
        fragmentShaderDesc.entryPoint = "main";
        fragmentShaderDesc.binary = shaderData.fragmentBinary;

        auto* device = renderer->GetDevice();
        auto* vertexShader = device->CreateShader(vertexShaderDesc);
        auto* fragmentShader = device->CreateShader(fragmentShaderDesc);

        auto shaderAsset = std::make_unique<GPUShaderAsset>(asset->GetUUID(),
                                                            asset->GetVersion(),
                                                            renderer,
                                                            vertexShader,
                                                            fragmentShader,
                                                            asset->GetReflection(),
                                                            renderer->GetCurrentFrameIndex());

        const auto& reflection = shaderAsset->GetReflection();

        renderer->RegisterShader(shaderAsset->GetUUID(), shaderAsset->GetSourceVersion(), reflection);

        return shaderAsset;
    }
} // namespace Hazel
