//
// Created by helmholtz on 2026/3/13.
//

#pragma once

#include "RHICommon.h"

namespace Hazel
{
    struct RHIInstanceDesc
    {
        RHIBackend backend = RHIBackend::Auto;
        std::string appName = "";
        Version appVersion = {1, 0, 0};
        std::string engineName = "";
        Version engineVersion = {1, 0, 0};

        bool useValidation = false;
        bool useCustomDebugMessenger = false;
        DebugMessageType debugMessageType = DebugMessageTypeFlagBits::General;
        DebugMessageSeverity debugMessageSeverity = DebugMessageSeverityFlagBits::Warning;

        DebugMessageCallback debugMessageCallback = nullptr;
    };
} // Hazel
