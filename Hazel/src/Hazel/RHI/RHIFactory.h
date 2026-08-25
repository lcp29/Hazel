// Declares the RHI factory interface.
// Created: 2026-03-13.

#pragma once

#include "RHIImage.h"
#include "RHIInstance.h"
#include "RHIQueue.h"
#include "RHIShader.h"

#include <filesystem>
#include <vector>

namespace Aster
{
    struct RHIShaderMacroDefinition
    {
        std::string name;
        std::string value;
    };

    struct RHIShaderFileDesc
    {
        std::filesystem::path path;
        RHIShaderStageFlagBits stage = RHIShaderStageFlagBits::Vertex;
        std::string entryPoint = "main";
        std::string debugName;
        std::vector<RHIShaderMacroDefinition> macroDefinitions;
    };

    std::optional<std::unique_ptr<RHIInstance>> CreateInstance(const RHIInstanceDesc& desc);
    RHIShader* CreateShaderFromGLSLFile(RHIDevice* device, const RHIShaderFileDesc& desc);
} // namespace Aster
