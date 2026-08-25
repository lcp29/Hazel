// Implements the Vulkan queue backend.
// Created: 2026-03-16.

#include "VulkanQueue.h"

#include "VulkanDevice.h"

namespace Aster
{
    namespace
    {
        constexpr auto s_DefaultSubmitStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    }

    RHI_VK_FUNC_IMPL(RHIQueue, RHIQueueImpl)(
        RHIDevice* device, RHIQueueTypes type, uint32_t familyIndex, vk::Queue queue, uint32_t queueIndex)
        : m_Type(type)
        , m_FamilyIndex(familyIndex)
        , m_QueueIndex(queueIndex)
        , m_Queue(queue)
    {
        m_DeviceOwner = device;

        if (!device) { return; }

        vk::SemaphoreTypeCreateInfo typeInfo;
        typeInfo.semaphoreType = vk::SemaphoreType::eTimeline;
        typeInfo.initialValue = 0;
        vk::SemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.pNext = &typeInfo;

        auto result = m_DeviceOwner->GetHandle().createSemaphore(&semaphoreCreateInfo, nullptr, &m_SignalSemaphore);

        m_TimelineValue = 0;

        if (result != vk::Result::eSuccess) { return; }

        m_IsValid = true;
    }

    RHISyncPoint RHI_VK_FUNC_IMPL(RHIQueue, Submit)(const RHIQueueSubmitDesc& desc)
    {
        std::scoped_lock lock(m_QueueSubmitMutex);
        m_TimelineValue++;

        vk::SubmitInfo2 submitInfo;
        submitInfo.commandBufferInfoCount = desc.commandBuffers.size();
        std::vector<vk::CommandBufferSubmitInfo> commandBufferInfos;
        commandBufferInfos.reserve(submitInfo.commandBufferInfoCount);
        for (const auto cb : desc.commandBuffers)
        {
            commandBufferInfos.emplace_back(cb->GetHandle());
        }
        submitInfo.pCommandBufferInfos = commandBufferInfos.data();

        submitInfo.waitSemaphoreInfoCount = 0;

        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreInfos;
        waitSemaphoreInfos.reserve(submitInfo.waitSemaphoreInfoCount);
        for (auto& waitSyncPoint : desc.waitSyncPoints)
        {
            if (waitSyncPoint.valid && waitSyncPoint.queue)
            {
                waitSemaphoreInfos.emplace_back(waitSyncPoint.queue->GetSignalSemaphore(), waitSyncPoint.value);
            }
        }
        submitInfo.waitSemaphoreInfoCount = waitSemaphoreInfos.size();
        submitInfo.pWaitSemaphoreInfos = waitSemaphoreInfos.data();

        vk::SemaphoreSubmitInfo signalSemaphoreInfo;
        signalSemaphoreInfo.semaphore = m_SignalSemaphore;
        signalSemaphoreInfo.value = m_TimelineValue;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

        auto result = m_Queue.submit2(1, &submitInfo, VK_NULL_HANDLE);
        if (result != vk::Result::eSuccess) { return {}; }

        RHISyncPoint syncPoint;
        syncPoint.value = m_TimelineValue;
        syncPoint.queue = this;
        syncPoint.valid = true;

        return syncPoint;
    }

    RHISyncPoint RHI_VK_FUNC_IMPL(RHIQueue, SignalOnBinarySemaphore)(vk::Semaphore semaphore)
    {
        HZ_RHI_DEBUG_RETURN_VALUE_IF(!m_IsValid || !semaphore, {});

        std::scoped_lock lock(m_QueueSubmitMutex);

        m_TimelineValue++;

        vk::SemaphoreSubmitInfo waitSemaphoreInfo;
        waitSemaphoreInfo.semaphore = semaphore;
        waitSemaphoreInfo.stageMask = s_DefaultSubmitStageMask;

        vk::SemaphoreSubmitInfo signalSemaphoreInfo;
        signalSemaphoreInfo.semaphore = m_SignalSemaphore;
        signalSemaphoreInfo.value = m_TimelineValue;
        signalSemaphoreInfo.stageMask = s_DefaultSubmitStageMask;

        vk::SubmitInfo2 submitInfo;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

        if (m_Queue.submit2(1, &submitInfo, VK_NULL_HANDLE) != vk::Result::eSuccess) { return {}; }

        return {m_TimelineValue, this, true};
    }

    bool RHI_VK_FUNC_IMPL(RHIQueue, WaitSyncPointsAndSignalBinary)(const std::vector<RHISyncPoint>& waitSyncPoints,
                                                                   vk::Semaphore semaphore)
    {
        HZ_RHI_DEBUG_FAIL_IF(!m_IsValid || !semaphore);

        std::scoped_lock lock(m_QueueSubmitMutex);

        vk::SemaphoreSubmitInfo signalSemaphoreInfo;
        signalSemaphoreInfo.semaphore = semaphore;
        signalSemaphoreInfo.stageMask = s_DefaultSubmitStageMask;

        vk::SubmitInfo2 submitInfo;
        submitInfo.waitSemaphoreInfoCount = static_cast<uint32_t>(waitSyncPoints.size());
        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreInfos;
        waitSemaphoreInfos.reserve(submitInfo.waitSemaphoreInfoCount);
        for (const auto& waitSyncPoint : waitSyncPoints)
        {
            HZ_RHI_DEBUG_FAIL_IF(!waitSyncPoint.valid || !waitSyncPoint.queue);

            waitSemaphoreInfos.emplace_back(
                waitSyncPoint.queue->GetSignalSemaphore(), waitSyncPoint.value, s_DefaultSubmitStageMask);
        }

        submitInfo.pWaitSemaphoreInfos = waitSemaphoreInfos.data();
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

        return m_Queue.submit2(1, &submitInfo, VK_NULL_HANDLE) == vk::Result::eSuccess;
    }

    RHI_VK_FUNC_IMPL(RHIQueue, ~RHIQueueImpl)() { ReleaseFromOwner(); }

    void RHI_VK_FUNC_IMPL(RHIQueue, ReleaseFromOwner)()
    {
        if (m_DeviceOwner && m_SignalSemaphore) { m_DeviceOwner->GetHandle().destroySemaphore(m_SignalSemaphore); }

        m_SignalSemaphore = VK_NULL_HANDLE;
        m_IsValid = false;
        m_Queue = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
    }
} // namespace Aster
