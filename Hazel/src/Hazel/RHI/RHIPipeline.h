//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include "Flags.h"
#include "RHIBase.h"
#include "RHICommon.h"
#include "RHIImage.h"
#include "RHIShader.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Hazel
{
    enum class RHIPrimitiveTopology : uint8_t
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip
    };

    enum class RHIPolygonMode : uint8_t
    {
        Fill,
        Line
    };

    enum class RHICullMode : uint8_t
    {
        None,
        Front,
        Back
    };

    enum class RHIFrontFace : uint8_t
    {
        CounterClockwise,
        Clockwise
    };

    enum class RHICompareOp : uint8_t
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class RHIBlendFactor : uint8_t
    {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha
    };

    enum class RHIBlendOp : uint8_t
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum class RHIColorComponentFlagBits : uint8_t
    {
        R = 1 << 0,
        G = 1 << 1,
        B = 1 << 2,
        A = 1 << 3
    };

    template <> struct InRHIFlagScope<RHIColorComponentFlagBits> : std::true_type
    {};

    using RHIColorComponentFlags = Flags<RHIColorComponentFlagBits>;

    enum class RHIPipelineStageFlagBits : uint32_t
    {
        Top = 1 << 0,
        DrawIndirect = 1 << 1,
        VertexInput = 1 << 2,
        VertexShader = 1 << 3,
        EarlyDepthStencil = 1 << 4,
        FragmentShader = 1 << 5,
        LateDepthStencil = 1 << 6,
        ColorAttachmentOutput = 1 << 7,
        ComputeShader = 1 << 8,
        Transfer = 1 << 9,
        Host = 1 << 10,
        AllGraphics = 1 << 11,
        AllCommands = 1 << 12,
        Bottom = 1 << 13
    };

    template <> struct InRHIFlagScope<RHIPipelineStageFlagBits> : std::true_type
    {};

    using RHIPipelineStages = Flags<RHIPipelineStageFlagBits>;

    // we are basically copying the Vulkan access flags
    enum class RHIPipelineAccessFlagBits : uint32_t
    {
        None = 1 << 0,
        IndirectCommandRead = 1 << 1,
        IndexRead = 1 << 2,
        VertexAttributeRead = 1 << 3,
        UniformRead = 1 << 4,
        InputAttachmentRead = 1 << 5,
        ShaderRead = 1 << 6,
        ShaderWrite = 1 << 7,
        ColorAttachmentRead = 1 << 8,
        ColorAttachmentWrite = 1 << 9,
        DepthStencilAttachmentRead = 1 << 10,
        DepthStencilAttachmentWrite = 1 << 11,
        TransferRead = 1 << 12,
        TransferWrite = 1 << 13,
        HostRead = 1 << 14,
        HostWrite = 1 << 15,
        MemoryRead = 1 << 16,
        MemoryWrite = 1 << 17,
        ShaderSampledRead = 1 << 18,
        ShaderStorageRead = 1 << 19,
        ShaderStorageWrite = 1 << 20
    };

    template <> struct InRHIFlagScope<RHIPipelineAccessFlagBits> : std::true_type
    {};

    using RHIPipelineAccessFlags = Flags<RHIPipelineAccessFlagBits>;

    enum class RHIVertexInputRate : uint8_t
    {
        Vertex,
        Instance
    };

    struct RHIVertexBindingDesc
    {
        uint32_t binding = 0;
        uint32_t stride = 0;
        RHIVertexInputRate inputRate = RHIVertexInputRate::Vertex;
    };

    struct RHIVertexAttributeDesc
    {
        uint32_t location = 0;
        uint32_t binding = 0;
        RHIFormat format = RHIFormat::Undefined;
        uint32_t offset = 0;
    };

    struct RHIColorBlendAttachmentDesc
    {
        bool blendEnable = false;
        RHIBlendFactor srcColorBlendFactor = RHIBlendFactor::One;
        RHIBlendFactor dstColorBlendFactor = RHIBlendFactor::Zero;
        RHIBlendOp colorBlendOp = RHIBlendOp::Add;
        RHIBlendFactor srcAlphaBlendFactor = RHIBlendFactor::One;
        RHIBlendFactor dstAlphaBlendFactor = RHIBlendFactor::Zero;
        RHIBlendOp alphaBlendOp = RHIBlendOp::Add;
        RHIColorComponentFlags colorWriteMask = RHIColorComponentFlagBits::R | RHIColorComponentFlagBits::G
                                                | RHIColorComponentFlagBits::B | RHIColorComponentFlagBits::A;

        bool operator==(const RHIColorBlendAttachmentDesc&) const = default;
    };

    struct RHIGraphicsPipelineDesc
    {
        RHIResourceSignature* resourceSignature = nullptr;
        RHIShader* vertexShader = nullptr;
        RHIShader* fragmentShader = nullptr;
        std::vector<RHIVertexBindingDesc> vertexBindings;
        std::vector<RHIVertexAttributeDesc> vertexAttributes;
        RHIPrimitiveTopology topology = RHIPrimitiveTopology::TriangleList;
        RHIPolygonMode polygonMode = RHIPolygonMode::Fill;
        RHICullMode cullMode = RHICullMode::Back;
        RHIFrontFace frontFace = RHIFrontFace::CounterClockwise;
        bool depthClampEnable = false;
        bool depthBiasEnable = false;
        bool depthTestEnable = false;
        bool depthWriteEnable = false;
        RHICompareOp depthCompareOp = RHICompareOp::LessOrEqual;
        bool stencilTestEnable = false;
        uint32_t sampleCount = 1;
        std::vector<RHIFormat> colorAttachmentFormats;
        RHIFormat depthStencilFormat = RHIFormat::Undefined;
        std::vector<RHIColorBlendAttachmentDesc> colorBlendAttachments;
        std::string debugName;
    };

    struct RHIComputePipelineDesc
    {
        RHIResourceSignature* resourceSignature = nullptr;
        RHIShader* computeShader = nullptr;
        std::string debugName;
    };

    struct RHIMemoryBarrier
    {
        RHIPipelineStages srcStages;
        RHIPipelineStages dstStages;
        RHIPipelineAccessFlags srcAccess;
        RHIPipelineAccessFlags dstAccess;
    };

    struct RHIImageMemoryBarrier
    {
        RHIImage* image;
        RHIPipelineStages srcStages;
        RHIPipelineStages dstStages;
        RHIPipelineAccessFlags srcAccess;
        RHIPipelineAccessFlags dstAccess;
        RHIImageResourceState oldState;
        RHIImageResourceState newState;
        RHIQueue* srcQueue;
        RHIQueue* dstQueue;
        RHIImageSubresourceRange subresourceRange;
    };

    struct RHIBufferMemoryBarrier
    {
        RHIBuffer* buffer;
        RHIPipelineStages srcStages;
        RHIPipelineStages dstStages;
        RHIPipelineAccessFlags srcAccess;
        RHIPipelineAccessFlags dstAccess;
        uint64_t offset;
        uint64_t size;
        RHIQueue* srcQueue;
        RHIQueue* dstQueue;
    };

    struct RHIPipelineBarrierDesc
    {
        std::vector<RHIMemoryBarrier> memoryBarriers{};
        std::vector<RHIImageMemoryBarrier> imageBarriers{};
        std::vector<RHIBufferMemoryBarrier> bufferBarriers{};
    };
} // namespace Hazel