//
// Created by helmholtz on 2026/3/15.
//

#include "VulkanBuffer.h"

#include "VulkanBufferView.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanMemoryAllocator.h"

namespace Hazel
{
    namespace
    {
        VmaAllocationCreateInfo VulkanConvertAllocationCreateInfo(const RHIBufferDesc &desc)
        {
            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

            switch (desc.cpuAccess)
            {
                case RHIBufferCpuAccess::None:
                    break;
                case RHIBufferCpuAccess::Write:
                    allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                    break;
                case RHIBufferCpuAccess::Read:
                    allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
                    break;
                case RHIBufferCpuAccess::ReadWrite:
                    allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
                    allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                    break;
            }

            if (desc.mapOnCreate)
            {
                allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            }

            return allocationCreateInfo;
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHIBuffer, RHIBufferImpl)(RHIDevice *deviceOwner,
                                               VulkanMemoryAllocator *allocatorOwner,
                                               const RHIBufferDesc &desc)
    {
        m_DeviceOwner = deviceOwner;
        m_AllocatorOwner = allocatorOwner;
        m_Desc = desc;
        m_PersistentMapping = desc.mapOnCreate;

        if (!m_DeviceOwner || !m_AllocatorOwner || desc.size == 0)
        {
            return;
        }

        vk::BufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.size = desc.size;
        bufferCreateInfo.usage = VulkanConvertBufferUsages(desc.usages);
        bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

        if (desc.allowGpuAddress)
        {
            bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
        }

        if (bufferCreateInfo.usage == vk::BufferUsageFlags())
        {
            bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        }

        auto allocationCreateInfo = VulkanConvertAllocationCreateInfo(desc);
        VmaAllocationInfo allocationInfo{};

        VkBuffer buffer = VK_NULL_HANDLE;
        const VkBufferCreateInfo vkBufferCreateInfo = bufferCreateInfo;
        if (!m_AllocatorOwner->CreateBuffer(
            vkBufferCreateInfo,
            allocationCreateInfo,
            &buffer,
            &m_Allocation,
            &allocationInfo))
        {
            m_Allocation = VK_NULL_HANDLE;
            return;
        }

        m_Buffer = buffer;
        m_MappedData = allocationInfo.pMappedData;

        if (desc.allowGpuAddress)
        {
            vk::BufferDeviceAddressInfo addressInfo;
            addressInfo.buffer = m_Buffer;
            m_DeviceAddress = m_DeviceOwner->GetHandle().getBufferAddress(addressInfo);
        }

        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIBuffer, ~RHIBufferImpl)()
    {
        Release();
    }

    void *RHI_VK_FUNC_IMPL(RHIBuffer, Map)()
    {
        if (!m_IsValid)
        {
            return nullptr;
        }

        if (m_MappedData)
        {
            return m_MappedData;
        }

        if (!m_AllocatorOwner)
        {
            return nullptr;
        }

        m_MappedData = m_AllocatorOwner->MapMemory(m_Allocation);
        return m_MappedData;
    }

    void RHI_VK_FUNC_IMPL(RHIBuffer, Unmap)()
    {
        if (!m_IsValid || !m_MappedData || !m_AllocatorOwner || m_PersistentMapping)
        {
            return;
        }

        m_AllocatorOwner->UnmapMemory(m_Allocation);
        m_MappedData = nullptr;
    }

    void RHI_VK_FUNC_IMPL(RHIBuffer, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterBuffer(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIBuffer, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterBuffer(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIBuffer, ReleaseWithoutUnregister)()
    {
        for (const auto &view: m_Views)
        {
            if (view)
            {
                view->ReleaseWithoutUnregister();
            }
        }
        m_Views.clear();

        auto allocator = m_AllocatorOwner ? m_AllocatorOwner->GetHandle() : VK_NULL_HANDLE;
        const auto buffer = static_cast<VkBuffer>(m_Buffer);
        const auto allocation = m_Allocation;

        if (!m_PersistentMapping && m_MappedData && m_AllocatorOwner)
        {
            m_AllocatorOwner->UnmapMemory(m_Allocation);
        }

        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([allocator, buffer, allocation]()
            {
                VulkanMemoryAllocator::DestroyBuffer(allocator, buffer, allocation);
            });
        }
        else
        {
            VulkanMemoryAllocator::DestroyBuffer(allocator, buffer, allocation);
        }

        m_Buffer = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
        m_MappedData = nullptr;
        m_DeviceAddress = 0;
        m_IsValid = false;
        m_PersistentMapping = false;
        m_DeviceOwner = nullptr;
        m_AllocatorOwner = nullptr;
    }

    void RHI_VK_FUNC_IMPL(RHIBuffer, ReleaseImmediateWithoutUnregister)()
    {
        for (const auto &view: m_Views)
        {
            if (view)
            {
                view->ReleaseImmediateWithoutUnregister();
            }
        }
        m_Views.clear();

        auto allocator = m_AllocatorOwner ? m_AllocatorOwner->GetHandle() : VK_NULL_HANDLE;
        const auto buffer = static_cast<VkBuffer>(m_Buffer);
        const auto allocation = m_Allocation;

        if (!m_PersistentMapping && m_MappedData && m_AllocatorOwner)
        {
            m_AllocatorOwner->UnmapMemory(m_Allocation);
        }

        VulkanMemoryAllocator::DestroyBuffer(allocator, buffer, allocation);

        m_Buffer = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
        m_MappedData = nullptr;
        m_DeviceAddress = 0;
        m_IsValid = false;
        m_PersistentMapping = false;
        m_DeviceOwner = nullptr;
        m_AllocatorOwner = nullptr;
    }

    RHIBufferView *RHI_VK_FUNC_IMPL(RHIBuffer, CreateView)(const RHIBufferViewDesc &desc)
    {
        if (!m_IsValid || !m_DeviceOwner)
        {
            return nullptr;
        }

        return m_DeviceOwner->CreateBufferView(this, desc);
    }

    void RHI_VK_FUNC_IMPL(RHIBuffer, RegisterView)(std::unique_ptr<RHIBufferView> view)
    {
        RegisterOwnedObject(m_Views, std::move(view));
    }

    void RHI_VK_FUNC_IMPL(RHIBuffer, UnregisterView)(RHIBufferView *view)
    {
        UnregisterOwnedObject(m_Views, view);
    }
} // Hazel
