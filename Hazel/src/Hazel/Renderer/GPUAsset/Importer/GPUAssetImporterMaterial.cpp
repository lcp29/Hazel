// Implements GPU asset import for material.
// Created: 2026-04-04.

#include "../CachedMaterial.h"
#include "../GPUShaderAsset.h"
#include "GPUAssetImporter.h"
#include "Hazel/Asset/Asset.h"

#include <Hazel/Renderer/Renderer.h>

namespace Aster
{
    std::unique_ptr<CachedMaterial> ImportCachedMaterial(Hazel::Renderer* renderer, const MaterialAsset* asset)
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
                        Hazel::UUID samplerUUID = bindlessProperty.sampler;
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
                        Hazel::UUID textureUUID = bindlessProperty.texture;
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
                        Hazel::UUID samplerUUID = bindlessProperty.sampler;
                        Hazel::UUID imageUUID = bindlessProperty.texture;
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
} // namespace Aster
