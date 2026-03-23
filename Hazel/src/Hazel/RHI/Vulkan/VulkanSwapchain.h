//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHISwapchain)
    {
    public:
        bool IsValid() const
        {
            return m_IsValid;
        }

        RHISwapchainAcquireResult AcquireImage();
        RHIImage* FetchImage(uint32_t frameNumber) const;
        RHIImageView* FetchImageView(uint32_t frameNumber) const;
        bool SubmitFrame(uint32_t frameNumber, const std::vector<RHISyncPoint>& waitSyncPoints);
        void Release();
        void ReleaseImmediate();
        ~RHISwapchainImpl();

        const RHISwapchainDesc& GetDesc() const
        {
            return m_Desc;
        }

        RHIFormat GetFormat() const
        {
            return m_Format;
        }

        uint32_t GetImageCount() const
        {
            return m_ImageCount;
        }

        bool IsDetached() const
        {
            return m_IsDetached;
        }

        vk::SwapchainKHR GetHandle() const
        {
            return m_Swapchain;
        }

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;

        RHISwapchainImpl(vk::PhysicalDevice physicalDevice,
                         RHIDevice* deviceOwner,

                         const RHISwapchainDesc& desc,
                         uint32_t presentQueueFamilyIndex);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        RHIDevice* m_DeviceOwner = nullptr;
        RHISwapchainDesc m_Desc;
        RHIFormat m_Format = RHIFormat::Undefined;
        uint32_t m_ImageCount = 0;
        vk::Device m_Device;
        vk::SwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<std::unique_ptr<RHIImage>> m_Images;
        std::vector<RHIImageView*> m_ImageViews;
        bool m_IsDetached = false;
        std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
        std::vector<vk::Semaphore> m_PresentSemaphores;
        uint32_t m_NextAcquireSemaphoreIndex = 0;
    };
} // namespace Hazel