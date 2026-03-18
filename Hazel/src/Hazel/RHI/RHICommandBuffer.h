//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "RHIBase.h"
#include "RHIImage.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Hazel
{
    enum class RHICommandBufferLevel : uint8_t
    {
        Primary,
        Secondary
    };

    struct RHICommandBufferDesc
    {
        RHICommandBufferLevel level = RHICommandBufferLevel::Primary;
    };

    enum class RHIIndexType : uint8_t
    {
        UInt16,
        UInt32
    };

    enum class RHIRenderingLoadOp : uint8_t
    {
        Load,
        Clear,
        DontCare
    };

    enum class RHIRenderingStoreOp : uint8_t
    {
        Store,
        DontCare
    };

    struct RHIRenderingColorAttachmentDesc
    {
        RHIImageView *imageView = nullptr;
        RHIImageResourceState state = RHIImageResourceState::ColorAttachment;
        RHIRenderingLoadOp loadOp = RHIRenderingLoadOp::Load;
        RHIRenderingStoreOp storeOp = RHIRenderingStoreOp::Store;
        float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    struct RHIRenderingDepthStencilAttachmentDesc
    {
        RHIImageView *imageView = nullptr;
        RHIImageResourceState state = RHIImageResourceState::DepthStencilAttachment;
        RHIRenderingLoadOp depthLoadOp = RHIRenderingLoadOp::Load;
        RHIRenderingStoreOp depthStoreOp = RHIRenderingStoreOp::Store;
        RHIRenderingLoadOp stencilLoadOp = RHIRenderingLoadOp::Load;
        RHIRenderingStoreOp stencilStoreOp = RHIRenderingStoreOp::Store;
        float clearDepth = 1.0f;
        uint32_t clearStencil = 0;
    };

    struct RHIRenderingInfo
    {
        std::vector<RHIRenderingColorAttachmentDesc> colorAttachments;
        std::optional<RHIRenderingDepthStencilAttachmentDesc> depthStencilAttachment;
        int32_t renderAreaX = 0;
        int32_t renderAreaY = 0;
        uint32_t renderAreaWidth = 0;
        uint32_t renderAreaHeight = 0;
        uint32_t layerCount = 1;
        uint32_t viewMask = 0;
    };
} // namespace Hazel

