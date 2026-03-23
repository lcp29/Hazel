//
// Created by helmholtz on 2026/3/16.
//

#define VULKAN_HPP_NO_EXCEPTIONS

#include "VulkanComputePipeline.h"

#include "VulkanDevice.h"
#include "VulkanPipelineCommon.h"
#include "VulkanResourceSignature.h"
#include "VulkanShader.h"

namespace Hazel
{
    RHI_VK_FUNC_IMPL(RHIComputePipeline, RHIComputePipelineImpl)(RHIDevice *deviceOwner,
                                                                 vk::Device device,
                                                                 const RHIComputePipelineDesc &desc)
    {
        m_DeviceOwner = deviceOwner;
        m_Device = device;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_Device || !desc.computeShader)
        {
            return;
        }

        auto *computeShader = desc.computeShader;
        HZ_RHI_DEBUG_RETURN_IF(!computeShader || !computeShader->IsValid());

        auto *resourceSignature = desc.resourceSignature;
        HZ_RHI_DEBUG_RETURN_IF(!resourceSignature || !resourceSignature->IsValid());
        m_ResourceSignature = resourceSignature;

        vk::PipelineShaderStageCreateInfo shaderStage(
            {},
            vk::ShaderStageFlagBits::eCompute,
            computeShader->GetHandle(),
            desc.computeShader->GetEntryPoint().c_str());

        vk::ComputePipelineCreateInfo createInfo;
        createInfo.stage = shaderStage;
        createInfo.layout = m_ResourceSignature->GetPipelineLayout();

        auto pipelineResult = m_Device.createComputePipeline(VK_NULL_HANDLE, createInfo);
        if (pipelineResult.result != vk::Result::eSuccess || !pipelineResult.value)
        {
            return;
        }

        m_Pipeline = pipelineResult.value;
        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIComputePipeline, ~RHIComputePipelineImpl)()
    {
        Release();
    }

    void RHI_VK_FUNC_IMPL(RHIComputePipeline, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterComputePipeline(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIComputePipeline, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterComputePipeline(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIComputePipeline, ReleaseWithoutUnregister)()
    {
        const auto device = m_Device;
        const auto pipeline = m_Pipeline;
        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device, pipeline]()
            {
                if (device && pipeline)
                {
                    device.destroyPipeline(pipeline);
                }
            });
        }
        else if (device && pipeline)
        {
            device.destroyPipeline(pipeline);
        }

        m_Pipeline = VK_NULL_HANDLE;
        m_ResourceSignature = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
        m_IsValid = false;
    }

    void RHI_VK_FUNC_IMPL(RHIComputePipeline, ReleaseImmediateWithoutUnregister)()
    {
        if (m_Device && m_Pipeline)
        {
            m_Device.destroyPipeline(m_Pipeline);
        }

        m_Pipeline = VK_NULL_HANDLE;
        m_ResourceSignature = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
        m_IsValid = false;
    }
} // Hazel
