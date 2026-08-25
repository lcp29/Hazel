// Declares the Vulkan resource signature backend.
// Created: 2026-03-16.

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Aster
{
    RHI_VK_CLASS_IMPL(RHIResourceSignature)
    {
      public:
        bool IsValid() const { return m_IsValid; }

        void Release();
        void ReleaseImmediate();
        ~RHIResourceSignatureImpl();

        const RHIResourceSignatureDesc& GetDesc() const { return m_Desc; }

        vk::PipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

        bool IsDetached() const { return m_IsDetached; }

      private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHICommandBufferImpl<RHIBackend::Vulkan>;
        friend class RHIGraphicsPipelineImpl<RHIBackend::Vulkan>;
        friend class RHIComputePipelineImpl<RHIBackend::Vulkan>;

        RHIResourceSignatureImpl(RHIDevice * deviceOwner, vk::Device device, const RHIResourceSignatureDesc& desc);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        RHIResourceSignatureDesc m_Desc;
        RHIDevice* m_DeviceOwner = nullptr;
        vk::Device m_Device;
        vk::PipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        bool m_IsDetached = false;
    };
} // namespace Aster
