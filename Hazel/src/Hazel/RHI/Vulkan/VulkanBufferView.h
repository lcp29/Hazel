// Declares the Vulkan buffer view backend.
// Created: 2026-03-15.

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Aster
{
    RHI_FORWARD_DECL_CLASS(RHIBuffer)

    RHI_VK_CLASS_IMPL(RHIBufferView)
    {
      public:
        bool IsValid() const { return m_IsValid; }

        void Release();
        void ReleaseImmediate();
        ~RHIBufferViewImpl();

        const RHIBufferViewDesc& GetDesc() const { return m_Desc; }

        RHIFormat GetFormat() const { return m_Desc.format; }

        uint64_t GetOffset() const { return m_Desc.offset; }

        uint64_t GetRange() const { return m_Desc.range; }

        vk::BufferView GetHandle() const { return m_BufferView; }

      private:
        friend class RHIBufferImpl<RHIBackend::Vulkan>;
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;

        RHIBufferViewImpl(
            RHIDevice * deviceOwner, RHIBuffer * bufferOwner, const RHIBufferViewDesc& desc, bool isDetached);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        RHIBufferViewDesc m_Desc;
        RHIDevice* m_DeviceOwner = nullptr;
        RHIBuffer* m_BufferOwner = nullptr;
        vk::BufferView m_BufferView = VK_NULL_HANDLE;
        bool m_IsDetached = false;
    };
} // namespace Aster
