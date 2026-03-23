//
// Created by helmholtz on 2026/3/13.
//

#pragma once

#include "RHIImage.h"
#include "RHIInstance.h"
#include "RHIQueue.h"
#include "RHIShader.h"

#include <filesystem>

namespace Hazel
{
    struct RHIShaderFileDesc
    {
        std::filesystem::path path;
        RHIShaderStageFlagBits stage = RHIShaderStageFlagBits::Vertex;
        std::string entryPoint = "main";
        std::string debugName;
    };

    std::optional<std::unique_ptr<RHIInstance>> CreateInstance(const RHIInstanceDesc& desc);
    RHIShader* CreateShaderFromGLSLFile(RHIDevice& device, const RHIShaderFileDesc& desc);
} // namespace Hazel