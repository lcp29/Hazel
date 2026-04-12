//
// Created by helmholtz on 2026/3/15.
//

#include "VulkanBufferView.h"

#include "VulkanBuffer.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"

namespace Hazel
{
    RHI_VK_FUNC_IMPL(RHIBufferView, RHIBufferViewImpl)(RHIDevice* device,
                                                       RHIBuffer* buffer,
                                                       const RHIBufferViewDesc& desc,
                                                       bool isDetached)
    {
        m_DeviceOwner = device;
        m_BufferOwner = buffer;
        m_Desc = desc;
        m_IsDetached = isDetached;

        if (!m_DeviceOwner || !m_BufferOwner || desc.format == RHIFormat::Undefined) { return; }

        HZ_RHI_DEBUG_RETURN_IF(!m_BufferOwner->IsValid());

        const auto resolvedRange = desc.range == 0 ? VK_WHOLE_SIZE : desc.range;

        vk::BufferViewCreateInfo createInfo;
        createInfo.buffer = m_BufferOwner->GetHandle();
        createInfo.format = VulkanConvertFormat(desc.format);
        createInfo.offset = desc.offset;
        createInfo.range = resolvedRange;

        auto result = m_DeviceOwner->GetHandle().createBufferView(&createInfo, nullptr, &m_BufferView);
        if (result != vk::Result::eSuccess)
        {
            m_BufferView = VK_NULL_HANDLE;
            return;
        }

        if (!m_IsDetached)
        {
            std::unique_ptr<RHIBufferView> self(this);
            m_BufferOwner->RegisterView(std::move(self));
        }
        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIBufferView, ~RHIBufferViewImpl)() { Release(); }

    void RHI_VK_FUNC_IMPL(RHIBufferView, Release)()
    {
        if (!m_IsValid) { return; }

        auto* bufferOwner = m_BufferOwner;
        ReleaseWithoutUnregister();
        if (bufferOwner && !m_IsDetached) { bufferOwner->UnregisterView(this); }
    }

    void RHI_VK_FUNC_IMPL(RHIBufferView, ReleaseImmediate)()
    {
        if (!m_IsValid) { return; }

        auto* bufferOwner = m_BufferOwner;
        ReleaseImmediateWithoutUnregister();
        if (bufferOwner && !m_IsDetached) { bufferOwner->UnregisterView(this); }
    }

    void RHI_VK_FUNC_IMPL(RHIBufferView, ReleaseWithoutUnregister)()
    {
        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device = m_DeviceOwner->GetHandle(), bufferView = m_BufferView]() {
                device.destroyBufferView(bufferView);
            });
        }

        m_DeviceOwner = nullptr;
        m_BufferOwner = nullptr;
        m_BufferView = VK_NULL_HANDLE;
        m_IsValid = false;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHIBufferView, ReleaseImmediateWithoutUnregister)()
    {
        if (m_DeviceOwner) { m_DeviceOwner->GetHandle().destroyBufferView(m_BufferView); }

        m_DeviceOwner = nullptr;
        m_BufferOwner = nullptr;
        m_BufferView = VK_NULL_HANDLE;
        m_IsValid = false;
        m_IsDetached = false;
    }
} // namespace Hazel