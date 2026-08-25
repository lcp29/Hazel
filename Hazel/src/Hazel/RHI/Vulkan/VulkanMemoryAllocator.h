// Declares the Vulkan memory allocator backend.
// Created: 2026-03-14.

#pragma once

#include "../RHIHeaders.h"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Aster
{
    class VulkanMemoryAllocator
    {
      public:
        VulkanMemoryAllocator(vk::Instance instance,
                              vk::PhysicalDevice physicalDevice,
                              vk::Device device,
                              const RHIDeviceCapabilities& capabilities);
        ~VulkanMemoryAllocator();

        VulkanMemoryAllocator(const VulkanMemoryAllocator&) = delete;
        VulkanMemoryAllocator& operator=(const VulkanMemoryAllocator&) = delete;

        bool IsValid() const { return m_IsValid; }

        VmaAllocator GetHandle() const { return m_Allocator; }

        bool CreateImage(const VkImageCreateInfo& imageCreateInfo,
                         const VmaAllocationCreateInfo& allocationCreateInfo,
                         VkImage* image,
                         VmaAllocation* allocation,
                         VmaAllocationInfo* allocationInfo = nullptr) const;
        bool CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo,
                          const VmaAllocationCreateInfo& allocationCreateInfo,
                          VkBuffer* buffer,
                          VmaAllocation* allocation,
                          VmaAllocationInfo* allocationInfo = nullptr) const;
        void* MapMemory(VmaAllocation allocation) const;
        void UnmapMemory(VmaAllocation allocation) const;
        static void DestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);
        static void DestroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation);
        VmaAllocator Detach();
        static void Destroy(VmaAllocator allocator);

      private:
        bool m_IsValid = false;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
    };
} // namespace Aster
