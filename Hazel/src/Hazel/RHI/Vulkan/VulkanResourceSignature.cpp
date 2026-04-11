//
// Created by helmholtz on 2026/3/16.
//

#define VULKAN_HPP_NO_EXCEPTIONS

#include "VulkanResourceSignature.h"

#include "VulkanDevice.h"
#include "VulkanPipelineCommon.h"

namespace Hazel
{
    RHI_VK_FUNC_IMPL(RHIResourceSignature, RHIResourceSignatureImpl)(RHIDevice* deviceOwner,
                                                                     vk::Device device,
                                                                     const RHIResourceSignatureDesc& desc)
    {
        m_DeviceOwner = deviceOwner;
        m_Device = device;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_Device)
        {
            return;
        }

        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<vk::PushConstantRange> pushConstantRanges;
        vk::PipelineLayoutCreateInfo createInfo;
        if (!VulkanBuildPipelineLayoutCreateInfo(desc, descriptorSetLayouts, pushConstantRanges, createInfo))
        {
            return;
        }

        auto pipelineLayoutResult = m_Device.createPipelineLayout(createInfo);
        if (pipelineLayoutResult.result != vk::Result::eSuccess || !pipelineLayoutResult.value)
        {
            return;
        }

        m_PipelineLayout = pipelineLayoutResult.value;
        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIResourceSignature, ~RHIResourceSignatureImpl)()
    {
        Release();
    }

    void RHI_VK_FUNC_IMPL(RHIResourceSignature, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner && !m_IsDetached)
        {
            deviceOwner->UnregisterResourceSignature(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIResourceSignature, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner && !m_IsDetached)
        {
            deviceOwner->UnregisterResourceSignature(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIResourceSignature, ReleaseWithoutUnregister)()
    {
        const auto device = m_Device;
        const auto pipelineLayout = m_PipelineLayout;
        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device, pipelineLayout]() {
                if (device && pipelineLayout)
                {
                    device.destroyPipelineLayout(pipelineLayout);
                }
            });
        }
        else if (device && pipelineLayout)
        {
            device.destroyPipelineLayout(pipelineLayout);
        }

        m_PipelineLayout = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHIResourceSignature, ReleaseImmediateWithoutUnregister)()
    {
        if (m_Device && m_PipelineLayout)
        {
            m_Device.destroyPipelineLayout(m_PipelineLayout);
        }

        m_PipelineLayout = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_IsDetached = false;
    }
} // namespace Hazel