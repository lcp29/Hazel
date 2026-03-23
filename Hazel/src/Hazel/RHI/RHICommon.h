//
// Created by helmholtz on 2026/3/13.
//

#pragma once

#include "Flags.h"
#include "RHIBase.h"

#include <cstdint>

#if defined(HZ_DEBUG)
#define HZ_RHI_DEBUG_FAIL_IF(condition)                                                                            \
    if (condition)                                                                                                 \
    {                                                                                                              \
        return false;                                                                                              \
    }

#define HZ_RHI_DEBUG_RETURN_NULL_IF(condition)                                                                     \
    if (condition)                                                                                                 \
    {                                                                                                              \
        return nullptr;                                                                                            \
    }

#define HZ_RHI_DEBUG_RETURN_IF(condition)                                                                          \
    if (condition)                                                                                                 \
    {                                                                                                              \
        return;                                                                                                    \
    }

#define HZ_RHI_DEBUG_RETURN_VALUE_IF(condition, value)                                                             \
    if (condition)                                                                                                 \
    {                                                                                                              \
        return value;                                                                                              \
    }
#else
#define HZ_RHI_DEBUG_FAIL_IF(condition)
#define HZ_RHI_DEBUG_RETURN_NULL_IF(condition)
#define HZ_RHI_DEBUG_RETURN_IF(condition)
#define HZ_RHI_DEBUG_RETURN_VALUE_IF(condition, value)
#endif

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

    template <>
    struct InRHIFlagScope<DebugMessageSeverityFlagBits> : std::true_type {};

    using DebugMessageSeverity = Flags<DebugMessageSeverityFlagBits>;

    enum class DebugMessageTypeFlagBits : uint8_t
    {
        General = 1 << 0,
        Performance = 1 << 1,
        Validation = 1 << 2
    };

    template <>
    struct InRHIFlagScope<DebugMessageTypeFlagBits> : std::true_type {};

    using DebugMessageType = Flags<DebugMessageTypeFlagBits>;

    struct DebugMessage
    {
        RHIBackend backend;

        DebugMessageType type;
        DebugMessageSeverity severity;

        int32_t messageIdNumber;
        const char* messageIdName;
        const char* message;
    };

    using DebugMessageCallback = void (*)(const DebugMessage&, void*);
} // namespace Hazel