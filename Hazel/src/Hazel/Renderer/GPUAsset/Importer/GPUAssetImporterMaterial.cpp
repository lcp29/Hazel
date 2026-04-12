//
// Created by helmholtz on 2026/4/4.
//

#include "../CachedMaterial.h"
#include "../GPUShaderAsset.h"
#include "GPUAssetImporter.h"
#include "Hazel/Asset/Asset.h"

#include <Hazel/Renderer/Renderer.h>

namespace Hazel
{
    std::unique_ptr<CachedMaterial> ImportCachedMaterial(Renderer* renderer, const MaterialAsset* asset)
    {
        auto shaderResult = renderer->ResolveGPUAssetBlocked(asset->GetMeta().GetShader(), AssetType::Shader);
        if (!shaderResult.asset) { return nullptr; }
        auto shader = static_cast<GPUShaderAsset*>(shaderResult.asset);

        std::unordered_map<std::string, MaterialAssetProperty> properties;
        for (const auto& property : asset->GetMeta().GetProperties())
        {
            MaterialAssetProperty bindlessProperty = property;
            switch (bindlessProperty.type)
            {
                case MaterialAssetPropertyType::Sampler:
                    {
                        UUID samplerUUID = bindlessProperty.sampler;
                        auto samplerResult = renderer->ResolveGPUAssetBlocked(samplerUUID, AssetType::Sampler);
                        uint32_t slot = samplerResult.asset
                                            ? renderer->RegisterBindlessSampler(std::move(samplerResult))
                                            : renderer->GetDefaultSamplerBindingSlot();
                        std::memcpy(&bindlessProperty.data, &slot, sizeof(uint32_t));
                        bindlessProperty.bindlessID = slot;
                        bindlessProperty.member.size = sizeof(uint32_t);
                        break;
                    }
                case MaterialAssetPropertyType::Texture:
                    {
                        UUID textureUUID = bindlessProperty.texture;
                        auto textureResult = renderer->ResolveGPUAssetBlocked(textureUUID, AssetType::Texture);
                        uint32_t slot = textureResult.asset
                                            ? renderer->RegisterBindlessTexture(std::move(textureResult))
                                            : renderer->GetWhiteTextureBindingSlot();
                        std::memcpy(&bindlessProperty.data, &slot, sizeof(uint32_t));
                        bindlessProperty.bindlessID = slot;
                        bindlessProperty.member.size = sizeof(uint32_t);
                        break;
                    }
                case MaterialAssetPropertyType::SamplerWithTexture:
                    {
                        UUID samplerUUID = bindlessProperty.sampler;
                        UUID imageUUID = bindlessProperty.texture;
                        auto samplerResult = renderer->ResolveGPUAssetBlocked(samplerUUID, AssetType::Sampler);
                        auto imageResult = renderer->ResolveGPUAssetBlocked(imageUUID, AssetType::Texture);
                        uint32_t slot = (samplerResult.asset && imageResult.asset)
                                            ? renderer->RegisterBindlessSamplerWithImage(std::move(imageResult),
                                                                                         std::move(samplerResult))
                                            : renderer->GetWhiteTextureWithDefaultSamplerBindingSlot();
                        std::memcpy(&bindlessProperty.data, &slot, sizeof(uint32_t));
                        bindlessProperty.bindlessID = slot;
                        bindlessProperty.member.size = sizeof(uint32_t);
                        break;
                    }
                default:
                    break;
            }
            properties[property.name] = bindlessProperty;
        }

        auto materialID = renderer->RegisterMaterial(shader->GetUUID(), shader->GetSourceVersion(), asset->GetUUID());

        auto material = std::make_unique<CachedMaterial>(asset->GetUUID(),
                                                         asset->GetVersion(),
                                                         materialID,
                                                         renderer,
                                                         asset->GetShader(),
                                                         shader->GetSourceVersion(),
                                                         asset->GetPipelineState(),
                                                         std::move(properties),
                                                         renderer->GetCurrentFrameIndex());
        return material;
    }
} // namespace Hazel