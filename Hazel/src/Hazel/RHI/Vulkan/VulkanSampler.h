//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHISampler)
    {
    public:
        bool IsValid() const
        {
            return m_IsValid;
        }

        void Release();
        void ReleaseImmediate();
        ~RHISamplerImpl();

        const RHISamplerDesc& GetDesc() const
        {
            return m_Desc;
        }

        vk::Sampler GetHandle() const
        {
            return m_Sampler;
        }

        bool IsDetached() const
        {
            return m_IsDetached;
        }

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIResourceGroupImpl<RHIBackend::Vulkan>;

        RHISamplerImpl(RHIDevice* deviceOwner, vk::Device device, const RHISamplerDesc& desc);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        RHISamplerDesc m_Desc;
        RHIDevice* m_DeviceOwner = nullptr;
        vk::Device m_Device;
        vk::Sampler m_Sampler = VK_NULL_HANDLE;
        bool m_IsDetached = false;
    };
} // namespace Hazel