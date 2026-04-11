//
// Created by helmholtz on 2026/3/14.
//

#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator.h"

namespace Hazel
{
    VulkanMemoryAllocator::VulkanMemoryAllocator(vk::Instance instance,
                                                 vk::PhysicalDevice physicalDevice,
                                                 vk::Device device,
                                                 const RHIDeviceCapabilities& capabilities)
    {
        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo createInfo{};
        createInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        createInfo.instance = static_cast<VkInstance>(instance);
        createInfo.physicalDevice = static_cast<VkPhysicalDevice>(physicalDevice);
        createInfo.device = static_cast<VkDevice>(device);
        createInfo.pVulkanFunctions = &vulkanFunctions;
        createInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        m_IsValid = vmaCreateAllocator(&createInfo, &m_Allocator) == VK_SUCCESS;
    }

    bool VulkanMemoryAllocator::CreateImage(const VkImageCreateInfo& imageCreateInfo,
                                            const VmaAllocationCreateInfo& allocationCreateInfo,
                                            VkImage* image,
                                            VmaAllocation* allocation,
                                            VmaAllocationInfo* allocationInfo) const
    {
        return m_Allocator
               && vmaCreateImage(
                   m_Allocator,
                   &imageCreateInfo,
                   &allocationCreateInfo,
                   image,
                   allocation,
                   allocationInfo)
               == VK_SUCCESS;
    }

    bool VulkanMemoryAllocator::CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo,
                                             const VmaAllocationCreateInfo& allocationCreateInfo,
                                             VkBuffer* buffer,
                                             VmaAllocation* allocation,
                                             VmaAllocationInfo* allocationInfo) const
    {
        return m_Allocator
               && vmaCreateBuffer(
                   m_Allocator,
                   &bufferCreateInfo,
                   &allocationCreateInfo,
                   buffer,
                   allocation,
                   allocationInfo)
               == VK_SUCCESS;
    }

    void* VulkanMemoryAllocator::MapMemory(VmaAllocation allocation) const
    {
        void* mappedData = nullptr;
        if (m_Allocator && allocation
            && vmaMapMemory(m_Allocator, allocation, &mappedData)

            == VK_SUCCESS)
        {
            return mappedData;
        }

        return nullptr;
    }

    void VulkanMemoryAllocator::UnmapMemory(VmaAllocation allocation) const
    {
        if (m_Allocator && allocation)
        {
            vmaUnmapMemory(m_Allocator, allocation);
        }
    }

    VulkanMemoryAllocator::~VulkanMemoryAllocator()
    {
        Destroy(m_Allocator);
        m_Allocator = VK_NULL_HANDLE;

        m_IsValid = false;
    }

    VmaAllocator VulkanMemoryAllocator::Detach()
    {
        auto allocator = m_Allocator;
        m_Allocator = VK_NULL_HANDLE;
        m_IsValid = false;
        return allocator;
    }

    void VulkanMemoryAllocator::Destroy(VmaAllocator allocator)
    {
        if (allocator)
        {
            vmaDestroyAllocator(allocator);
        }
    }

    void VulkanMemoryAllocator::DestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
    {
        if (allocator && buffer && allocation)
        {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }

    void VulkanMemoryAllocator::DestroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation)
    {
        if (allocator && image && allocation)
        {
            vmaDestroyImage(allocator, image, allocation);
        }
    }
} // namespace Hazel