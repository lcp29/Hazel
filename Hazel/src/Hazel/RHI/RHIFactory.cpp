//
// Created by helmholtz on 2026/3/13.
//

#include "RHIFactory.h"

#include "RHI.h"

#include <cstring>
#include <fstream>
#include <shaderc/shaderc.hpp>

namespace Hazel
{
    namespace
    {
        shaderc_shader_kind ToShadercShaderKind(const RHIShaderStageFlagBits stage)
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

        std::string ReadTextFile(const std::filesystem::path& path)
        {
            std::ifstream input(path, std::ios::in | std::ios::binary);
            if (!input)
            {
                return {};
            }

            input.seekg(0, std::ios::end);
            const auto size = input.tellg();
            if (size <= 0)
            {
                return {};
            }

            std::string contents(size, '\0');
            input.seekg(0, std::ios::beg);
            input.read(contents.data(), size);
            return contents;
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
    }

    std::optional<std::unique_ptr<RHIInstance>> CreateInstance(const RHIInstanceDesc& desc)
    {
        switch (desc.backend)
        {
            case RHIBackend::Auto:
            case RHIBackend::Vulkan:
            {
                auto instance = std::make_unique<RHIInstance>(desc);
                return instance->IsValid() ? std::make_optional(std::move(instance)) : std::nullopt;
            }
        }
        return std::nullopt;
    }

    RHIShader* CreateShaderFromGLSLFile(RHIDevice* device, const RHIShaderFileDesc& desc)
    {
        if (desc.path.empty())
        {
            return nullptr;
        }

        const auto source = ReadTextFile(desc.path);
        if (source.empty())
        {
            return nullptr;
        }

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
        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            return nullptr;
        }

        RHIShaderDesc shaderDesc;
        shaderDesc.stage = desc.stage;
        shaderDesc.entryPoint = desc.entryPoint;
        shaderDesc.debugName = desc.debugName.empty() ? desc.path.filename().string() : desc.debugName;
        shaderDesc.binary.assign(result.cbegin(), result.cend());

        return device->CreateShader(shaderDesc);
    }
} // namespace Hazel