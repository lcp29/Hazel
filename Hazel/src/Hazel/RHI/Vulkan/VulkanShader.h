//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHIShader)
    {
    public:
        bool IsValid() const
        {
            return m_IsValid;
        }

        void Release();
        void ReleaseImmediate();
        ~RHIShaderImpl();

        const RHIShaderDesc& GetDesc() const
        {
            return m_Desc;
        }

        RHIShaderStageFlagBits GetStage() const
        {
            return m_Desc.stage;
        }

        const std::string& GetEntryPoint() const
        {
            return m_Desc.entryPoint;
        }

        const std::string& GetDebugName() const
        {
            return m_Desc.debugName;
        }

        const RHIShaderReflection& GetReflection() const
        {
            return m_Reflection;
        }

        vk::ShaderModule GetHandle() const
        {
            return m_ShaderModule;
        }

        bool IsDetached() const
        {
            return m_IsDetached;
        }

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIGraphicsPipelineImpl<RHIBackend::Vulkan>;
        friend class RHIComputePipelineImpl<RHIBackend::Vulkan>;

        RHIShaderImpl(RHIDevice* deviceOwner, vk::Device device, const RHIShaderDesc& desc);

        bool Reflect();
        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        RHIShaderDesc m_Desc;
        RHIShaderReflection m_Reflection;
        RHIDevice* m_DeviceOwner = nullptr;
        vk::Device m_Device;
        vk::ShaderModule m_ShaderModule = VK_NULL_HANDLE;
        bool m_IsDetached = false;
    };
} // namespace Hazel