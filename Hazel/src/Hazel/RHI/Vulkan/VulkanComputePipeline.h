//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"
#include "VulkanResourceLayout.h"
#include "VulkanResourceSignature.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHIComputePipeline)
    {
    public:
        bool IsValid() const
        {
            return m_IsValid;
        }

        void Release();
        void ReleaseImmediate();
        ~RHIComputePipelineImpl();

        const RHIComputePipelineDesc& GetDesc() const
        {
            return m_Desc;
        }

        vk::Pipeline GetHandle() const
        {
            return m_Pipeline;
        }

        vk::PipelineLayout GetPipelineLayout() const
        {
            return m_ResourceSignature ? m_ResourceSignature->GetPipelineLayout() : VK_NULL_HANDLE;
        }

        bool IsDetached() const
        {
            return m_IsDetached;
        }

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHICommandBufferImpl<RHIBackend::Vulkan>;

        RHIComputePipelineImpl(RHIDevice* deviceOwner, vk::Device device, const RHIComputePipelineDesc& desc);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        RHIComputePipelineDesc m_Desc;
        RHIDevice* m_DeviceOwner = nullptr;
        RHIResourceSignature* m_ResourceSignature = nullptr;
        vk::Device m_Device;
        vk::Pipeline m_Pipeline = VK_NULL_HANDLE;
        bool m_IsDetached = false;
    };
} // namespace Hazel