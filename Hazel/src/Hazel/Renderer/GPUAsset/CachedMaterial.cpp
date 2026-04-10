//
// Created by helmholtz on 2026/4/1.
//

#include "CachedMaterial.h"

#include <ranges>

namespace Hazel
{
    namespace
    {
        uint64_t HashCombine(uint64_t seed, uint64_t value)
        {
            return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
        }

        uint64_t HashColorBlendAttachment(const RHIColorBlendAttachmentDesc& attachment)
        {
            uint64_t seed = std::hash<bool>{}(attachment.blendEnable);
            seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(attachment.srcColorBlendFactor)));
            seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(attachment.dstColorBlendFactor)));
            seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(attachment.colorBlendOp)));
            seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(attachment.srcAlphaBlendFactor)));
            seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(attachment.dstAlphaBlendFactor)));
            seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(attachment.alphaBlendOp)));
            seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint8_t>(attachment.colorWriteMask)));
            return seed;
        }
    }

    uint64_t CachedMaterial::GetPipelineKey(const std::vector<RHIFormat>& colorAttachmentFormats,
                                            const std::vector<RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                                            RHIFormat depthStencilFormat) const
    {
        uint64_t seed = std::hash<UUID>{}(m_Shader);
        seed = HashCombine(seed, std::hash<uint64_t>{}(m_ShaderSourceVersion));

        seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(m_PipelineState.polygonMode)));
        seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(m_PipelineState.cullMode)));
        seed = HashCombine(seed, std::hash<bool>{}(m_PipelineState.depthClampEnable));
        seed = HashCombine(seed, std::hash<bool>{}(m_PipelineState.depthBiasEnable));
        seed = HashCombine(seed, std::hash<bool>{}(m_PipelineState.depthTestEnable));
        seed = HashCombine(seed, std::hash<bool>{}(m_PipelineState.depthWriteEnable));
        seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(m_PipelineState.depthCompareOp)));
        seed = HashCombine(seed, std::hash<bool>{}(m_PipelineState.stencilTestEnable));

        seed = HashCombine(seed, std::hash<uint64_t>{}(GetSourceVersion()));

        for (const auto& attachment : colorBlendAttachments)
        {
            seed = HashCombine(seed, HashColorBlendAttachment(attachment));
        }

        for (const auto format : colorAttachmentFormats)
        {
            seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(format)));
        }

        seed = HashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(depthStencilFormat)));
        return seed;
    }

    void CachedMaterial::Release()
    {
        for (auto& property : m_Properties | std::views::values)
        {
            if (property.type == MaterialAssetPropertyType::Texture)
            {
                m_Renderer->UnregisterBindlessTexture(property.bindlessID);
            }
            else if (property.type == MaterialAssetPropertyType::Sampler)
            {
                m_Renderer->UnregisterBindlessSampler(property.bindlessID);
            }
            else if (property.type == MaterialAssetPropertyType::SamplerWithTexture)
            {
                m_Renderer->UnregisterBindlessSamplerWithImage(property.bindlessID);
            }
        }
        m_Renderer->UnregisterMaterial(m_Shader, m_ShaderSourceVersion, m_MaterialID);
    }

    void CachedMaterial::ReleaseImmediate()
    {
        m_Renderer->UnregisterMaterial(m_Shader, m_ShaderSourceVersion, m_MaterialID);
    }
} // Hazel
