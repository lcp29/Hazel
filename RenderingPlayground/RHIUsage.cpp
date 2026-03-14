//
// Created by helmholtz on 2026/3/14.
//

#include <iostream>
#include <string>
#include "../Hazel/src/Hazel/RHI/RHIInstance.h"
#include "Hazel/RHI/RHIDesc.h"
#include "Hazel/RHI/RHIFactory.h"

namespace Hazel
{
    void debugMessageCallback(const DebugMessage &msg, void *)
    {
        std::cout << msg.message << std::endl;
    }

    void test()
    {
        RHIInstanceDesc desc;
        desc.backend = RHIBackend::Vulkan;
        desc.appName = "RHI Usage Test";
        desc.appVersion = {1, 0, 0};
        desc.engineName = "Rendering Playground";
        desc.engineVersion = {1, 0, 0};
        desc.debugMessageSeverity = DebugMessageSeverityFlagBits::Verbose | DebugMessageSeverityFlagBits::Info |
                                    DebugMessageSeverityFlagBits::Warning | DebugMessageSeverityFlagBits::Error;
        desc.debugMessageType = DebugMessageTypeFlagBits::Validation;
        desc.debugMessageCallback = debugMessageCallback;

        auto r = CreateInstance(desc);
        std::cout << r.has_value() << std::endl;
        auto a = r.value()->GetAdapters();
        for (auto &c : a)
        {
            std::cout << c->GetName() << std::endl;
        }
    }
};

int main()
{
    Hazel::test();
    return 0;
}
