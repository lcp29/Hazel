//
// Created by helmholtz on 2026/3/13.
//

#include "RHIFactory.h"

#include "Vulkan/VulkanInstance.h"

namespace Hazel
{
    std::optional<Scope<RHIInstance>> CreateInstance(const RHIInstanceDesc &desc)
    {
        switch (desc.backend)
        {
            case RHIBackend::Auto:
            case RHIBackend::Vulkan:
                auto instance = CreateScope<VulkanInstance>(desc);
                return instance->IsValid() ? std::make_optional(std::move(instance)) : std::nullopt;
        }
        return std::nullopt;
    }
} // Hazel
