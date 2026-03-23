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

            std::string contents(static_cast<size_t>(size), '\0');
            input.seekg(0, std::ios::beg);
            input.read(contents.data(), static_cast<std::streamsize>(size));
            return contents;
        }

        RHIImageResourceState GetUploadFinalState(const RHIImageDesc& desc)
        {
            if (desc.initialState != RHIImageResourceState::Undefined)
            {
                return desc.initialState;
            }

            if (desc.usages & RHIImageUsageFlagBits::Sampled)
            {
                return RHIImageResourceState::ShaderRead;
            }
            if (desc.usages & RHIImageUsageFlagBits::ColorAttachment)
            {
                return RHIImageResourceState::ColorAttachment;
            }
            if (desc.usages & RHIImageUsageFlagBits::DepthStencilAttachment)
            {
                return RHIImageResourceState::DepthStencilAttachment;
            }

            return RHIImageResourceState::TransferDestination;
        }

        RHIPipelineStages GetStageMask(RHIImageResourceState state)
        {
            switch (state)
            {
            case RHIImageResourceState::Undefined:
                return RHIPipelineStageFlagBits::Top;
            case RHIImageResourceState::Common:
                return RHIPipelineStageFlagBits::AllCommands;
            case RHIImageResourceState::TransferSource:
            case RHIImageResourceState::TransferDestination:
                return RHIPipelineStageFlagBits::Transfer;
            case RHIImageResourceState::ShaderRead:
            case RHIImageResourceState::ShaderWrite:
                return RHIPipelineStageFlagBits::AllCommands;
            case RHIImageResourceState::ColorAttachment:
                return RHIPipelineStageFlagBits::ColorAttachmentOutput;
            case RHIImageResourceState::DepthStencilAttachment:
                return RHIPipelineStageFlagBits::EarlyDepthStencil | RHIPipelineStageFlagBits::LateDepthStencil;
            case RHIImageResourceState::Present:
                return RHIPipelineStageFlagBits::Bottom;
            }

            return RHIPipelineStageFlagBits::AllCommands;
        }

        vk::AccessFlags2 GetAccessMask(RHIImageResourceState state)
        {
            switch (state)
            {
            case RHIImageResourceState::Undefined:
                return {};
            case RHIImageResourceState::Common:
                return vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
            case RHIImageResourceState::TransferSource:
                return vk::AccessFlagBits2::eTransferRead;
            case RHIImageResourceState::TransferDestination:
                return vk::AccessFlagBits2::eTransferWrite;
            case RHIImageResourceState::ShaderRead:
                return vk::AccessFlagBits2::eShaderRead;
            case RHIImageResourceState::ShaderWrite:
                return vk::AccessFlagBits2::eShaderWrite;
            case RHIImageResourceState::ColorAttachment:
                return vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
            case RHIImageResourceState::DepthStencilAttachment:
                return vk::AccessFlagBits2::eDepthStencilAttachmentRead
                    | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
            case RHIImageResourceState::Present:
                return {};
            }

            return {};
        }

        vk::ImageAspectFlags GetAspectMask(RHIFormat format)
        {
            switch (format)
            {
            case RHIFormat::D32SFloat:
                return vk::ImageAspectFlagBits::eDepth;
            case RHIFormat::D32SFloatS8Uint:
                return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            default:
                return vk::ImageAspectFlagBits::eColor;
            }
        }
    } // namespace

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

    RHIShader* CreateShaderFromGLSLFile(RHIDevice& device, const RHIShaderFileDesc& desc)
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
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetSourceLanguage(shaderc_source_language_glsl);

        auto result = compiler.CompileGlslToSpv(
            source, ToShadercShaderKind(desc.stage), desc.path.string().c_str(), desc.entryPoint.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            return nullptr;
        }

        RHIShaderDesc shaderDesc;
        shaderDesc.stage = desc.stage;
        shaderDesc.entryPoint = desc.entryPoint;
        shaderDesc.debugName = desc.debugName.empty() ? desc.path.filename().string() : desc.debugName;
        shaderDesc.binary.assign(result.cbegin(), result.cend());

        return device.CreateShader(shaderDesc);
    }
} // namespace Hazel