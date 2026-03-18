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

    std::optional<std::unique_ptr<RHIInstance>> CreateInstance(const RHIInstanceDesc &desc);
    RHIImage *CreateImageFromLinearBuffer(RHIDevice &device,
                                          const RHIImageDesc &desc,
                                          const void *data,
                                          size_t dataSize,
                                          RHIQueue *queue = nullptr);
    RHIShader *CreateShaderFromGLSLFile(RHIDevice &device, const RHIShaderFileDesc &desc);
} // Hazel
