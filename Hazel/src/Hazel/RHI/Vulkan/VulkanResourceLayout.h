//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHIResourceLayout)
    {
    public:
        bool IsValid() const { return m_IsValid; }
        void Release();
        void ReleaseImmediate();
        ~RHIResourceLayoutImpl();

        const RHIResourceLayoutDesc &GetDesc() const { return m_Desc; }
        vk::DescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIResourceSignatureImpl<RHIBackend::Vulkan>;
        friend class RHIResourceGroupImpl<RHIBackend::Vulkan>;

        RHIResourceLayoutImpl(RHIDevice *deviceOwner, vk::Device device, const RHIResourceLayoutDesc &desc);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        RHIResourceLayoutDesc m_Desc;
        RHIDevice *m_DeviceOwner = nullptr;
        vk::Device m_Device;
        vk::DescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    };
} // Hazel
