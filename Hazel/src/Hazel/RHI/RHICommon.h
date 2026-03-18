//
// Created by helmholtz on 2026/3/13.
//

#pragma once

#include <cstdint>

#include "Flags.h"
#include "RHIBase.h"

namespace Hazel
{
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

    template<>
    struct InRHIFlagScope<DebugMessageSeverityFlagBits> : std::true_type {};

    using DebugMessageSeverity = Flags<DebugMessageSeverityFlagBits>;

    enum class DebugMessageTypeFlagBits : uint8_t
    {
        General = 1 << 0,
        Performance = 1 << 1,
        Validation = 1 << 2
    };

    template<>
    struct InRHIFlagScope<DebugMessageTypeFlagBits> : std::true_type {};

    using DebugMessageType = Flags<DebugMessageTypeFlagBits>;

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
} // Hazel
