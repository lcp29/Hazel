//
// Created by helmholtz on 2026/3/13.
//

#pragma once

#include "Flags.h"

namespace Hazel
{
    enum class RHIBackend
    {
        Auto = 1 << 0,
        Vulkan = 1 << 1
    };

    struct Version
    {
        uint8_t major;
        uint8_t minor;
        uint8_t patch;
    };

    enum class DebugMessageSeverityFlagBits : uint8_t
    {
        Verbose = 1 << 0,
        Info = 1 << 1,
        Warning = 1 << 2,
        Error = 1 << 3
    };

    using DebugMessageSeverity = Flags<DebugMessageSeverityFlagBits>;

    enum class DebugMessageTypeFlagBits : uint8_t
    {
        General = 1 << 0,
        Performance = 1 << 1,
        Validation = 1 << 2
    };

    using DebugMessageType = Flags<DebugMessageTypeFlagBits>;

    inline DebugMessageType operator|(DebugMessageType a, DebugMessageTypeFlagBits b)
    {
        return a | DebugMessageType(b);
    }

    inline DebugMessageSeverity operator|(DebugMessageSeverity a, DebugMessageSeverityFlagBits b)
    {
        return a | DebugMessageSeverity(b);
    }

    struct DebugMessage
    {
        RHIBackend backend;

        DebugMessageType type;
        DebugMessageSeverity severity;

        int32_t messageIdNumber;
        const char *messageIdName;
        const char *message;
    };

    using DebugMessageCallback = void(*)(const DebugMessage &, void *);

    struct RHIDeviceCapabilities
    {
        bool supportGpuAddress;

        bool supportSubgroup;
        uint32_t subgroupSizeMin;
        uint32_t subgroupSizeMax;
    };
} // Hazel
