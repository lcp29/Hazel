//
// Created by helmholtz on 2026/3/31.
//

#pragma once
#include "Hazel/RHI/RHIFactory.h"

#include <shaderc/shaderc.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace Hazel
{
    inline std::string ReadTextFile(const std::filesystem::path& filePath)
    {
        std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
        if (!fileStream)
        {
            throw std::runtime_error("Failed to open file: " + filePath.string());
        }
        fileStream.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(fileStream.tellg());
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
            if (requestedPath.is_absolute())
            {
                filePath = requestedPath;
            }
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
            source,
            ToShadercShaderKind(desc.stage),
            desc.path.string().c_str(),
            desc.entryPoint.c_str(),
            options);

        return result;
    }
}