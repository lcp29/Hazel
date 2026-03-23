//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanCommon.h"
#include "VulkanResourceLayout.h"

#include <algorithm>
#include <vector>

namespace Hazel
{
    inline vk::PrimitiveTopology VulkanConvertPrimitiveTopology(RHIPrimitiveTopology topology)
    {
        switch (topology)
        {
#define X(rhi, vul)                                                                                                    \
    case RHIPrimitiveTopology::rhi:                                                                                    \
        return vk::PrimitiveTopology::vul;
#include "TypeMappings/PrimitiveTopologies.inl"

#undef X
            default:
                return vk::PrimitiveTopology::eTriangleList;
        }
    }

    inline vk::PolygonMode VulkanConvertPolygonMode(RHIPolygonMode mode)
    {
        switch (mode)
        {
#define X(rhi, vul)                                                                                                    \
    case RHIPolygonMode::rhi:                                                                                          \
        return vk::PolygonMode::vul;
#include "TypeMappings/PolygonModes.inl"

#undef X
            default:
                return vk::PolygonMode::eFill;
        }
    }

    inline vk::CullModeFlags VulkanConvertCullMode(RHICullMode mode)
    {
        switch (mode)
        {
#define X(rhi, vul)                                                                                                    \
    case RHICullMode::rhi:                                                                                             \
        return vk::CullModeFlagBits::vul;
#include "TypeMappings/CullModes.inl"

#undef X
            default:
                return vk::CullModeFlagBits::eBack;
        }
    }

    inline vk::FrontFace VulkanConvertFrontFace(RHIFrontFace face)
    {
        switch (face)
        {
#define X(rhi, vul)                                                                                                    \
    case RHIFrontFace::rhi:                                                                                            \
        return vk::FrontFace::vul;
#include "TypeMappings/FrontFaces.inl"

#undef X
            default:
                return vk::FrontFace::eCounterClockwise;
        }
    }

    inline vk::CompareOp VulkanConvertCompareOp(RHICompareOp op)
    {
        switch (op)
        {
#define X(rhi, vul)                                                                                                    \
    case RHICompareOp::rhi:                                                                                            \
        return vk::CompareOp::vul;
#include "TypeMappings/CompareOps.inl"

#undef X
            default:
                return vk::CompareOp::eLessOrEqual;
        }
    }

    inline vk::BlendFactor VulkanConvertBlendFactor(RHIBlendFactor factor)
    {
        switch (factor)
        {
#define X(rhi, vul)                                                                                                    \
    case RHIBlendFactor::rhi:                                                                                          \
        return vk::BlendFactor::vul;
#include "TypeMappings/BlendFactors.inl"

#undef X
            default:
                return vk::BlendFactor::eOne;
        }
    }

    inline vk::BlendOp VulkanConvertBlendOp(RHIBlendOp op)
    {
        switch (op)
        {
#define X(rhi, vul)                                                                                                    \
    case RHIBlendOp::rhi:                                                                                              \
        return vk::BlendOp::vul;
#include "TypeMappings/BlendOps.inl"

#undef X
            default:
                return vk::BlendOp::eAdd;
        }
    }

    inline vk::ColorComponentFlags VulkanConvertColorComponentFlags(RHIColorComponentFlags flags)
    {
        vk::ColorComponentFlags result;

#define X(rhi, vul)                                                                                                    \
    if (flags & RHIColorComponentFlagBits::rhi)                                                                        \
    {                                                                                                                  \
        result |= vk::ColorComponentFlagBits::vul;                                                                     \
    }
#include "TypeMappings/ColorComponentFlags.inl"

#undef X
        return result;
    }

    inline vk::VertexInputRate VulkanConvertVertexInputRate(RHIVertexInputRate rate)
    {
        switch (rate)
        {
#define X(rhi, vul)                                                                                                    \
    case RHIVertexInputRate::rhi:                                                                                      \
        return vk::VertexInputRate::vul;
#include "TypeMappings/VertexInputRates.inl"

#undef X
            default:
                return vk::VertexInputRate::eVertex;
        }
    }

    inline vk::SampleCountFlagBits VulkanConvertSampleCount(uint32_t sampleCount)
    {
        switch (sampleCount)
        {
            case 1:
                return vk::SampleCountFlagBits::e1;
            case 2:
                return vk::SampleCountFlagBits::e2;
            case 4:
                return vk::SampleCountFlagBits::e4;
            case 8:
                return vk::SampleCountFlagBits::e8;
            case 16:
                return vk::SampleCountFlagBits::e16;
            case 32:
                return vk::SampleCountFlagBits::e32;
            case 64:
                return vk::SampleCountFlagBits::e64;
            default:
                return vk::SampleCountFlagBits::e1;
        }
    }

    inline bool VulkanBuildPipelineLayoutCreateInfo(const RHIResourceSignatureDesc& desc,
                                                    std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
                                                    std::vector<vk::PushConstantRange>& pushConstantRanges,
                                                    vk::PipelineLayoutCreateInfo& createInfo)
    {
        descriptorSetLayouts.clear();
        pushConstantRanges.clear();
        descriptorSetLayouts.reserve(desc.resourceLayouts.size());

        for (auto* layout : desc.resourceLayouts)
        {
            auto* vkLayout = layout;
            if (!vkLayout || !vkLayout->IsValid())
            {
                return false;
            }

            descriptorSetLayouts.push_back(vkLayout->GetDescriptorSetLayout());
        }

        for (const auto& range : desc.pushConstantRanges)
        {
            const vk::PushConstantRange vkRange(VulkanConvertShaderStages(range.stages), range.offset, range.size);

            const auto duplicate = std::find_if(pushConstantRanges.begin(),
                                                pushConstantRanges.end(),
                                                [&vkRange](const vk::PushConstantRange& existingRange) {
                                                    return existingRange.stageFlags == vkRange.stageFlags
                                                           && existingRange.offset == vkRange.offset
                                                           && existingRange.size == vkRange.size;
                                                });
            if (duplicate == pushConstantRanges.end())
            {
                pushConstantRanges.push_back(vkRange);
            }
        }

        createInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        createInfo.pSetLayouts = descriptorSetLayouts.data();
        createInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        createInfo.pPushConstantRanges = pushConstantRanges.data();
        return true;
    }
} // namespace Hazel