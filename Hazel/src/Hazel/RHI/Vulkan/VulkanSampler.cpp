//
// Created by helmholtz on 2026/3/16.
//

#include "VulkanSampler.h"

#include "VulkanDevice.h"
#include "VulkanPipelineCommon.h"

namespace Hazel
{
    namespace
    {
        vk::Filter VulkanConvertSamplerFilter(RHISamplerFilter filter)
        {
            switch (filter)
            {
                case RHISamplerFilter::Nearest:
                    return vk::Filter::eNearest;
                case RHISamplerFilter::Linear:
                    return vk::Filter::eLinear;
            }

            return vk::Filter::eLinear;
        }

        vk::SamplerMipmapMode VulkanConvertSamplerMipFilter(RHISamplerFilter filter)
        {
            switch (filter)
            {
                case RHISamplerFilter::Nearest:
                    return vk::SamplerMipmapMode::eNearest;
                case RHISamplerFilter::Linear:
                    return vk::SamplerMipmapMode::eLinear;
            }

            return vk::SamplerMipmapMode::eLinear;
        }

        vk::SamplerAddressMode VulkanConvertSamplerAddressMode(RHISamplerAddressMode mode)
        {
            switch (mode)
            {
                case RHISamplerAddressMode::Repeat:
                    return vk::SamplerAddressMode::eRepeat;
                case RHISamplerAddressMode::MirroredRepeat:
                    return vk::SamplerAddressMode::eMirroredRepeat;
                case RHISamplerAddressMode::ClampToEdge:
                    return vk::SamplerAddressMode::eClampToEdge;
                case RHISamplerAddressMode::ClampToBorder:
                    return vk::SamplerAddressMode::eClampToBorder;
            }

            return vk::SamplerAddressMode::eRepeat;
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHISampler, RHISamplerImpl)(RHIDevice* deviceOwner, vk::Device device, const RHISamplerDesc& desc)
    {
        m_DeviceOwner = deviceOwner;
        m_Device = device;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_Device) { return; }

        vk::SamplerCreateInfo createInfo;
        createInfo.magFilter = VulkanConvertSamplerFilter(desc.magFilter);
        createInfo.minFilter = VulkanConvertSamplerFilter(desc.minFilter);
        createInfo.mipmapMode = VulkanConvertSamplerMipFilter(desc.mipFilter);
        createInfo.addressModeU = VulkanConvertSamplerAddressMode(desc.addressModeU);
        createInfo.addressModeV = VulkanConvertSamplerAddressMode(desc.addressModeV);
        createInfo.addressModeW = VulkanConvertSamplerAddressMode(desc.addressModeW);
        createInfo.mipLodBias = desc.mipLodBias;
        createInfo.anisotropyEnable = desc.enableAnisotropy && desc.maxAnisotropy > 1.0f;
        createInfo.maxAnisotropy = desc.maxAnisotropy > 1.0f ? desc.maxAnisotropy : 1.0f;
        createInfo.minLod = desc.minLod;
        createInfo.maxLod = desc.maxLod;
        createInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
        createInfo.unnormalizedCoordinates = VK_FALSE;
        createInfo.compareEnable = desc.compareEnable;
        createInfo.compareOp = VulkanConvertCompareOp(desc.compareOp);

        auto result = m_Device.createSampler(&createInfo, nullptr, &m_Sampler);
        if (result != vk::Result::eSuccess)
        {
            m_Sampler = VK_NULL_HANDLE;
            return;
        }

        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHISampler, ~RHISamplerImpl)() { Release(); }

    void RHI_VK_FUNC_IMPL(RHISampler, Release)()
    {
        if (!m_IsValid) { return; }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner && !m_IsDetached) { deviceOwner->UnregisterSampler(this); }
    }

    void RHI_VK_FUNC_IMPL(RHISampler, ReleaseImmediate)()
    {
        if (!m_IsValid) { return; }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner && !m_IsDetached) { deviceOwner->UnregisterSampler(this); }
    }

    void RHI_VK_FUNC_IMPL(RHISampler, ReleaseWithoutUnregister)()
    {
        const auto device = m_Device;
        const auto sampler = m_Sampler;

        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device, sampler]() {
                if (device && sampler) { device.destroySampler(sampler); }
            });
        }
        else if (device && sampler) { device.destroySampler(sampler); }

        m_Sampler = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
        m_IsValid = false;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHISampler, ReleaseImmediateWithoutUnregister)()
    {
        if (m_Device && m_Sampler) { m_Device.destroySampler(m_Sampler); }

        m_Sampler = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
        m_IsValid = false;
        m_IsDetached = false;
    }
} // namespace Hazel