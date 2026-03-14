//
// Created by helmholtz on 2026/3/13.
//

#pragma once
#include "VulkanCommon.h"
#include "../RHIDesc.h"
#include "../RHIInstance.h"

#include <vulkan/vulkan.hpp>


namespace Hazel
{
    class VulkanInstance : public RHIInstance
    {
    public:
        bool IsValid() const override { return m_IsValid; }

        std::vector<Ref<RHIAdapter>> GetAdapters() override;

        VulkanInstance(const VulkanInstance &) = delete;

        VulkanInstance &operator=(const VulkanInstance &) = delete;

        VulkanInstance(const VulkanInstance &&instance) noexcept;

        VulkanInstance(const RHIInstanceDesc &desc);

        ~VulkanInstance() override;

    private:
        bool m_IsValid = false;
        vk::Instance m_Instance;
        RHIInstanceDesc m_InstanceDesc;
        VulkanDebugMessageContext m_DebugCallbackContext;
    };
} // Hazel
