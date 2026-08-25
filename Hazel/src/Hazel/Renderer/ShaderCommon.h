// Declares shared shader compilation and reflection utilities.
// Created: 2026-03-31.

#pragma once
#include "Hazel/RHI/RHIFactory.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <string>

namespace Aster
{
    constexpr int kPerViewResourceSet = 0;
    constexpr int kUserResourceSet = 1;
    constexpr int kMaterialResourceSet = 2;

    inline std::string ReadTextFile(const std::filesystem::path& filePath)
    {
        std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
        if (!fileStream)
        {
            HZ_WARN("Failed to open file: " + filePath.string());
            return {};
        }
        fileStream.seekg(0, std::ios::end);
        size_t fileSize = fileStream.tellg();
        fileStream.seekg(0, std::ios::beg);
        std::string fileContent(fileSize, '\0');
        fileStream.read(fileContent.data(), fileSize);
        fileStream.close();
        return fileContent;
    }

    class GLSLFileIncluder : public shaderc::CompileOptions::IncluderInterface
    {
      public:
        shaderc_include_result* GetInclude(const char* requested_source,
                                           shaderc_include_type type,
                                           const char* requesting_source,
                                           size_t include_depth) override
        {
            std::filesystem::path requestingPath(requesting_source);
            std::filesystem::path requestedPath(requested_source);
            std::filesystem::path filePath;
            if (requestedPath.is_absolute()) { filePath = requestedPath; }
            else
            {
                filePath = requestingPath.parent_path() / requestedPath;
            }
            auto requestedFileContent = ReadTextFile(filePath);

            auto* result = new shaderc_include_result;
            result->source_name = new char[filePath.string().size() + 1];
            std::strcpy(const_cast<char*>(result->source_name), filePath.string().c_str());
            result->source_name_length = filePath.string().size();
            result->content = new char[requestedFileContent.size() + 1];
            std::strcpy(const_cast<char*>(result->content), requestedFileContent.c_str());
            result->content_length = requestedFileContent.size();

            return result;
        }

        void ReleaseInclude(shaderc_include_result* data) override
        {
            delete[] data->source_name;
            delete[] data->content;
            delete data;
        }
    };

    inline shaderc_shader_kind ToShadercShaderKind(const RHIShaderStageFlagBits stage)
    {
        switch (stage)
        {
            case RHIShaderStageFlagBits::Vertex:
                return shaderc_glsl_vertex_shader;
            case RHIShaderStageFlagBits::Fragment:
                return shaderc_glsl_fragment_shader;
            case RHIShaderStageFlagBits::Compute:
                return shaderc_glsl_compute_shader;
        }

        return shaderc_glsl_infer_from_source;
    }

    inline shaderc::SpvCompilationResult CompileShaderFileToSPIRV(const RHIShaderFileDesc& desc)
    {
        const auto source = ReadTextFile(desc.path);
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
#ifdef RHI_USE_VULKAN
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
#endif
        options.SetSourceLanguage(shaderc_source_language_glsl);
        options.SetIncluder(std::make_unique<GLSLFileIncluder>());
        for (const auto& macroDefinition : desc.macroDefinitions)
        {
            options.AddMacroDefinition(macroDefinition.name, macroDefinition.value);
        }

        auto result = compiler.CompileGlslToSpv(
            source, ToShadercShaderKind(desc.stage), desc.path.string().c_str(), desc.entryPoint.c_str(), options);

        return result;
    }

    inline bool ReflectShaderSPIRV(const std::vector<uint32_t>& spirvBinary, RHIShaderReflection& reflection)
    { return RHIShader::Reflect(spirvBinary, reflection); }

    inline RHIShaderReflection MergeShaderReflections(const RHIShaderReflection& vertexReflection,
                                                      const RHIShaderReflection& fragmentReflection)
    {
        RHIShaderReflection mergedReflection = vertexReflection;

        for (const auto& fragmentGroup : fragmentReflection.resourceGroups)
        {
            auto groupIt = std::ranges::find_if(mergedReflection.resourceGroups,
                                                [&fragmentGroup](const RHIShaderResourceGroupReflection& group) {
                                                    return group.set == fragmentGroup.set;
                                                });

            if (groupIt == mergedReflection.resourceGroups.end())
            {
                mergedReflection.resourceGroups.push_back(fragmentGroup);
                continue;
            }

            for (const auto& fragmentSlot : fragmentGroup.slots)
            {
                auto slotIt =
                    std::ranges::find_if(groupIt->slots, [&fragmentSlot](const RHIShaderSlotReflection& slot) {
                        return slot.slot == fragmentSlot.slot;
                    });

                if (slotIt == groupIt->slots.end()) { groupIt->slots.push_back(fragmentSlot); }
            }
        }

        for (const auto& fragmentPushConstant : fragmentReflection.pushConstants)
        {
            auto pushConstantIt =
                std::ranges::find_if(mergedReflection.pushConstants,
                                     [&fragmentPushConstant](const RHIShaderPushConstantReflection& pushConstant) {
                                         return pushConstant.offset == fragmentPushConstant.offset
                                                && pushConstant.size == fragmentPushConstant.size;
                                     });

            if (pushConstantIt == mergedReflection.pushConstants.end())
            {
                mergedReflection.pushConstants.push_back(fragmentPushConstant);
            }
        }

        std::ranges::sort(mergedReflection.resourceGroups,
                          [](const RHIShaderResourceGroupReflection& lhs, const RHIShaderResourceGroupReflection& rhs) {
                              return lhs.set < rhs.set;
                          });

        for (auto& group : mergedReflection.resourceGroups)
        {
            std::ranges::sort(group.slots, [](const RHIShaderSlotReflection& lhs, const RHIShaderSlotReflection& rhs) {
                return lhs.slot < rhs.slot;
            });
        }

        std::ranges::sort(mergedReflection.pushConstants,
                          [](const RHIShaderPushConstantReflection& lhs, const RHIShaderPushConstantReflection& rhs) {
                              return lhs.offset < rhs.offset;
                          });

        return mergedReflection;
    }

    inline void AddReflectionToSetData(std::vector<RHIResourceLayoutDesc>& setData,
                                       const RHIShaderReflection& reflection,
                                       RHIShaderStages stage)
    {
        for (const auto& group : reflection.resourceGroups)
        {
            if (group.set >= setData.size()) { setData.resize(group.set + 1); }

            auto& layoutDesc = setData[group.set];
            for (const auto& slot : group.slots)
            {
                auto bindingIt =
                    std::ranges::find_if(layoutDesc.bindings, [&slot](const RHIResourceBindingSlotDesc& binding) {
                        return binding.slot == slot.slot;
                    });

                if (bindingIt == layoutDesc.bindings.end())
                {
                    RHIResourceBindingSlotDesc bindingDesc{};
                    bindingDesc.slot = slot.slot;
                    bindingDesc.type = slot.type;
                    bindingDesc.count = slot.count;
                    bindingDesc.stages = stage;

                    if (group.set == kPerViewResourceSet && (slot.slot >= 1 && slot.slot <= 3))
                    {
                        bindingDesc.partiallyBound = true;
                    }

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
} // namespace Aster
