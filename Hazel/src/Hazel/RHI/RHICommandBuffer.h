//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "RHIBase.h"
#include "RHIImage.h"

#include <array>
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

    enum class RHIBlitFilter : uint8_t
    {
        Nearest,
        Linear
    };

    struct RHIOffset2D
    {
        int32_t x = 0;
        int32_t y = 0;
    };

    struct RHIExtent2D
    {
        uint32_t width = 0;
        uint32_t height = 0;
    };

    struct RHIOffset3D
    {
        int32_t x = 0;
        int32_t y = 0;
        int32_t z = 0;
    };

    struct RHIExtent3D
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 0;
    };

    struct RHIImageSubresourceLayers
    {
        uint32_t mipLevel = 0;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
        RHIImagePlanes planes = RHIImagePlaneFlagBits::Color;
    };

    struct RHIImageBlitRegion
    {
        RHIImageSubresourceLayers srcSubresource;
        RHIOffset3D srcOffsets[2] = {};
        RHIImageSubresourceLayers dstSubresource;
        RHIOffset3D dstOffsets[2] = {};
    };

    struct RHIImageBlitDesc
    {
        std::vector<RHIImageBlitRegion> regions;
        RHIBlitFilter filter = RHIBlitFilter::Linear;
    };

    struct RHIBufferCopyRegion
    {
        uint64_t srcOffset = 0;
        uint64_t dstOffset = 0;
        uint64_t size = 0;
    };

    struct RHIBufferCopyDesc
    {
        std::vector<RHIBufferCopyRegion> regions;
    };

    struct RHIClearColorValue
    {
        enum class Type
        {
            Float,
            Int,
            UInt
        } type = Type::Float;

        union
        {
            std::array<float, 4> float32 = {0.0f, 0.0f, 0.0f, 1.0f};
            std::array<int32_t, 4> int32;
            std::array<uint32_t, 4> uint32;
        };
    };

    struct RHIClearDepthStencilValue
    {
        float depth = 1.0f;
        uint32_t stencil = 0;
    };

    struct RHIClearAttachmentDesc
    {
        RHIImagePlanes planes = RHIImagePlaneFlagBits::Color;
        uint32_t colorAttachment = 0;
        RHIClearColorValue colorValue;
        RHIClearDepthStencilValue depthStencilValue;
    };

    struct RHIClearRect
    {
        RHIOffset2D offset = {0, 0};
        RHIExtent2D extent = {0, 0};
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
    };

    struct RHIClearAttachmentsDesc
    {
        std::vector<RHIClearAttachmentDesc> attachments;
        std::vector<RHIClearRect> rects;
    };

    struct RHIRenderingAttachmentDesc
    {
        RHIImageView* imageView = nullptr;
        RHIImageResourceState state = RHIImageResourceState::ColorAttachment;
        RHIRenderingLoadOp loadOp = RHIRenderingLoadOp::Load;
        RHIRenderingStoreOp storeOp = RHIRenderingStoreOp::Store;
        RHIClearColorValue clearColorValue;
        RHIClearDepthStencilValue clearDepthStencilValue;
    };

    struct RHIRenderingInfo
    {
        std::vector<RHIRenderingAttachmentDesc> colorAttachments;
        std::optional<RHIRenderingAttachmentDesc> depthAttachment;
        std::optional<RHIRenderingAttachmentDesc> stencilAttachment;
        RHIOffset2D renderOffset = {0, 0};
        RHIExtent2D renderViewSize = {0, 0};
        uint32_t layerCount = 1;
        uint32_t viewMask = 0;
    };
} // namespace Hazel