//
// Created by helmholtz on 2026/4/4.
//

#pragma once

#include "Hazel/RHI/RHI.h"

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <vector>

namespace Hazel::GPUAssetImporterInternal
{
    inline constexpr uint32_t kMaterialResourceSet = 2;

    inline void AddReflectionToSetData(std::vector<RHIResourceLayoutDesc>& setData,
                                       const RHIShaderReflection& reflection,
                                       RHIShaderStages stage)
    {
        for (const auto& group : reflection.resourceGroups)
        {
            if (group.set >= setData.size())
            {
                setData.resize(group.set + 1);
            }

            auto& layoutDesc = setData[group.set];
            for (const auto& slot : group.slots)
            {
                auto bindingIt = std::ranges::find_if(layoutDesc.bindings,
                                                      [&slot](const RHIResourceBindingSlotDesc& binding) {
                                                          return binding.slot == slot.slot;
                                                      });

                if (bindingIt == layoutDesc.bindings.end())
                {
                    RHIResourceBindingSlotDesc bindingDesc{};
                    bindingDesc.slot = slot.slot;
                    bindingDesc.type = slot.type;
                    bindingDesc.count = slot.count;
                    bindingDesc.stages = stage;
                    layoutDesc.bindings.push_back(bindingDesc);
                }
                else
                {
                    bindingIt->stages |= stage;
                    bindingIt->count = slot.count;
                }
            }
        }
    }

    inline std::vector<RHIPushConstantRangeDesc> BuildPushConstantRanges(const RHIShaderReflection& vertexReflection,
                                                                         const RHIShaderReflection& fragmentReflection)
    {
        std::vector<RHIPushConstantRangeDesc> ranges;

        auto appendRanges = [&ranges](const RHIShaderReflection& reflection, RHIShaderStages stages) {
            for (const auto& pushConstant : reflection.pushConstants)
            {
                auto it = std::ranges::find_if(ranges,
                                               [&pushConstant](const RHIPushConstantRangeDesc& range) {
                                                   return range.offset == pushConstant.offset &&
                                                          range.size == pushConstant.size;
                                               });
                if (it == ranges.end())
                {
                    RHIPushConstantRangeDesc range{};
                    range.offset = pushConstant.offset;
                    range.size = pushConstant.size;
                    range.stages = stages;
                    ranges.push_back(range);
                }
                else
                {
                    it->stages |= stages;
                }
            }
        };

        appendRanges(vertexReflection, RHIShaderStageFlagBits::Vertex);
        appendRanges(fragmentReflection, RHIShaderStageFlagBits::Fragment);
        return ranges;
    }

    inline std::vector<RHIPushConstantRangeDesc> BuildComputePushConstantRanges(const RHIShaderReflection& reflection)
    {
        std::vector<RHIPushConstantRangeDesc> ranges;
        for (const auto& pushConstant : reflection.pushConstants)
        {
            RHIPushConstantRangeDesc range{};
            range.offset = pushConstant.offset;
            range.size = pushConstant.size;
            range.stages = RHIShaderStageFlagBits::Compute;
            ranges.push_back(range);
        }
        return ranges;
    }
} // namespace Hazel::GPUAssetImporterInternal
