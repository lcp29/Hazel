//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"
#include "VulkanDevice.h"
#include "VulkanBufferView.h"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Hazel
{
    class VulkanMemoryAllocator;

    RHI_VK_CLASS_IMPL(RHIBuffer)
    {
    public:
        void *Map();
        void Unmap();
        bool IsMapped() const { return m_MappedData != nullptr; }
        bool IsValid() const { return m_IsValid; }
        uint64_t GetDeviceAddress() const { return m_DeviceAddress; }
        RHIBufferView *CreateView(const RHIBufferViewDesc &desc);
        void Release();
        void ReleaseImmediate();

        const RHIBufferDesc &GetDesc() const { return m_Desc; }
        uint64_t GetSize() const { return m_Desc.size; }
        RHIBufferUsages GetUsages() const { return m_Desc.usages; }
        RHIBufferCpuAccess GetCpuAccess() const { return m_Desc.cpuAccess; }

        vk::Buffer GetHandle() const { return m_Buffer; }
        VmaAllocation GetAllocation() const { return m_Allocation; }
        ~RHIBufferImpl();

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIBufferViewImpl<RHIBackend::Vulkan>;

        RHIBufferImpl(RHIDevice *deviceOwner, VulkanMemoryAllocator *allocatorOwner, const RHIBufferDesc &desc);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();
        void RegisterView(std::unique_ptr<RHIBufferView> view);
        void UnregisterView(RHIBufferView *view);

        bool m_IsValid = false;
        bool m_PersistentMapping = false;
        RHIBufferDesc m_Desc;
        RHIDevice *m_DeviceOwner = nullptr;
        VulkanMemoryAllocator *m_AllocatorOwner = nullptr;
        vk::Buffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        void *m_MappedData = nullptr;
        uint64_t m_DeviceAddress = 0;
        RHIOwnerSet<RHIBufferView> m_Views;
    };
} // Hazel
