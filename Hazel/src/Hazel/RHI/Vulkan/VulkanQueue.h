//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHIQueue)
    {
      public:
        RHIQueueType GetType() const { return m_Type; }

        vk::Queue GetHandle() const { return m_Queue; }

        uint32_t GetFamilyIndex() const { return m_FamilyIndex; }

        uint32_t GetQueueIndex() const { return m_QueueIndex; }

        uint64_t GetCurrentTimelineValue() const { return m_TimelineValue; }

        RHISyncPoint Submit(const RHIQueueSubmitDesc& desc);
        RHISyncPoint SignalOnBinarySemaphore(vk::Semaphore semaphore);
        bool WaitSyncPointsAndSignalBinary(const std::vector<RHISyncPoint>& waitSyncPoints, vk::Semaphore semaphore);

        bool IsValid() const { return m_IsValid; }

        ~RHIQueueImpl();

      private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;

        RHIQueueImpl(
            RHIDevice * device, RHIQueueType type, uint32_t familyIndex, vk::Queue queue, uint32_t queueIndex = 0);
        void ReleaseFromOwner();

        vk::Semaphore GetSignalSemaphore() const { return m_SignalSemaphore; }

        RHIQueueType m_Type;
        bool m_IsValid = false;
        vk::Semaphore m_SignalSemaphore = VK_NULL_HANDLE;
        uint32_t m_FamilyIndex;
        uint32_t m_QueueIndex;
        uint64_t m_TimelineValue;
        vk::Queue m_Queue;
        std::mutex m_QueueSubmitMutex;
        RHIDevice* m_DeviceOwner = nullptr;
    };
} // namespace Hazel
