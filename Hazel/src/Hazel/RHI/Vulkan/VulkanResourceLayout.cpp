//
// Created by helmholtz on 2026/3/15.
//

#include "VulkanResourceLayout.h"

#include "VulkanCommon.h"
#include "VulkanDevice.h"

namespace Hazel
{
    RHI_VK_FUNC_IMPL(RHIResourceLayout, RHIResourceLayoutImpl)(RHIDevice* deviceOwner,
                                                               vk::Device device,
                                                               const RHIResourceLayoutDesc& desc)
    {
        m_DeviceOwner = deviceOwner;
        m_Device = device;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_Device)
        {
            return;
        }

        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        bindings.reserve(desc.bindings.size());
        for (const auto& bindingDesc : desc.bindings)
        {
            bindings.emplace_back(bindingDesc.slot,
                                  VulkanConvertResourceBindingType(bindingDesc.type),
                                  bindingDesc.count,
                                  VulkanConvertShaderStages(bindingDesc.stages));
        }

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutCreateInfo.pBindings = bindings.data();
        m_DescriptorSetLayout = m_Device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
        if (!m_DescriptorSetLayout)
        {
            return;
        }

        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIResourceLayout, ~RHIResourceLayoutImpl)()
    {
        Release();
    }

    void RHI_VK_FUNC_IMPL(RHIResourceLayout, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner && !m_IsDetached)
        {
            deviceOwner->UnregisterResourceLayout(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIResourceLayout, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner && !m_IsDetached)
        {
            deviceOwner->UnregisterResourceLayout(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIResourceLayout, ReleaseWithoutUnregister)()
    {
        const auto device = m_Device;
        const auto descriptorSetLayout = m_DescriptorSetLayout;
        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device, descriptorSetLayout]() {
                if (device && descriptorSetLayout)
                {
                    device.destroyDescriptorSetLayout(descriptorSetLayout);
                }
            });
        }
        else if (device && descriptorSetLayout)
        {
            device.destroyDescriptorSetLayout(descriptorSetLayout);
        }

        m_DescriptorSetLayout = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHIResourceLayout, ReleaseImmediateWithoutUnregister)()
    {
        if (m_Device && m_DescriptorSetLayout)
        {
            m_Device.destroyDescriptorSetLayout(m_DescriptorSetLayout);
        }

        m_DescriptorSetLayout = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_IsDetached = false;
    }
} // namespace Hazel