//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "RHIBase.h"
#include "RHICommon.h"

#include <string>
#include <vector>

namespace Hazel
{
    enum class RHIShaderStageFlagBits : uint16_t
    {
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Compute = 1 << 2
    };

    template <> struct InRHIFlagScope<RHIShaderStageFlagBits> : std::true_type
    {};

    using RHIShaderStages = Flags<RHIShaderStageFlagBits>;

    enum class RHIResourceBindingType : uint8_t
    {
        Sampler,
        SamplerWithImage,
        SampledImage,
        StorageImage,
        UniformBuffer,
        StorageBuffer,
        UniformTexelBuffer,
        StorageTexelBuffer,
        UniformDynamicBuffer,
        StorageDynamicBuffer
    };

    enum class RHIShaderValueBaseType : uint8_t
    {
        Unknown,
        Boolean,
        SInt,
        UInt,
        Float,
        Struct
    };

    struct RHIShaderBufferMemberReflection
    {
        std::string name;
        std::string typeName;
        RHIShaderValueBaseType baseType = RHIShaderValueBaseType::Unknown;
        uint32_t columns = 1;
        uint32_t rows = 1;
        uint32_t arraySize = 0;
        uint32_t size = 0;
        uint32_t offset = 0;
    };

    struct RHIShaderBufferReflection
    {
        std::string name;
        uint32_t size = 0;
        std::vector<RHIShaderBufferMemberReflection> members;
    };

    struct RHIShaderSlotReflection
    {
        uint32_t slot = 0;
        RHIResourceBindingType type = RHIResourceBindingType::UniformBuffer;
        uint32_t count = 1;
        std::string variableName;
        RHIShaderBufferReflection buffer;
    };

    struct RHIShaderResourceGroupReflection
    {
        uint32_t set = 0;
        std::vector<RHIShaderSlotReflection> slots;
    };

    struct RHIShaderPushConstantReflection
    {
        std::string name;
        uint32_t size = 0;
        uint32_t offset = 0;
        std::vector<RHIShaderBufferMemberReflection> members;
    };

    struct RHIShaderReflection
    {
        std::vector<RHIShaderResourceGroupReflection> resourceGroups;
        std::vector<RHIShaderPushConstantReflection> pushConstants;
    };

    struct RHIShaderDesc
    {
        RHIShaderStageFlagBits stage = RHIShaderStageFlagBits::Vertex;
        std::string entryPoint = "main";
        std::string debugName;
        std::vector<uint32_t> binary;
    };
} // namespace Hazel