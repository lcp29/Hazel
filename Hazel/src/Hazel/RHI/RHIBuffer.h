//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "RHIBase.h"
#include "RHIBufferView.h"
#include "RHICommon.h"

#include <cstdint>

namespace Hazel
{
    enum class RHIBufferCpuAccess : uint8_t
    {
        None = 0,
        Write = 1,
        Read = 2,
        ReadWrite = 3
    };

    inline RHIBufferCpuAccess operator|(RHIBufferCpuAccess lhs, RHIBufferCpuAccess rhs)
    {
        return static_cast<RHIBufferCpuAccess>(static_cast<std::underlying_type_t<RHIBufferCpuAccess>>(lhs)
            | static_cast<std::underlying_type_t<RHIBufferCpuAccess>>(rhs));
    }

    inline RHIBufferCpuAccess operator&(RHIBufferCpuAccess lhs, RHIBufferCpuAccess rhs)
    {
        return static_cast<RHIBufferCpuAccess>(static_cast<std::underlying_type_t<RHIBufferCpuAccess>>(lhs)
            & static_cast<std::underlying_type_t<RHIBufferCpuAccess>>(rhs));
    }

    enum class RHIBufferUsageFlagBits : uint16_t
    {
        Undefined = 1 << 0,
        TransferSource = 1 << 1,
        TransferDestination = 1 << 2,
        UniformTexelBuffer = 1 << 3,
        StorageTexelBuffer = 1 << 4,
        UniformBuffer = 1 << 5,
        StorageBuffer = 1 << 6,
        IndexBuffer = 1 << 7,
        VertexBuffer = 1 << 8,
        IndirectBuffer = 1 << 9
    };

    template <>
    struct InRHIFlagScope<RHIBufferUsageFlagBits> : std::true_type {};

    using RHIBufferUsages = Flags<RHIBufferUsageFlagBits>;

    struct RHIBufferDesc
    {
        uint64_t size = 0;
        RHIBufferUsages usages = {};
        RHIBufferCpuAccess cpuAccess = RHIBufferCpuAccess::None;
        bool mapOnCreate = false;
        bool allowGpuAddress = false;
        bool hostCoherent = false;
        bool deviceMemory = false;
    };
} // namespace Hazel