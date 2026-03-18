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
        bool IsValid() const { return m_IsValid; }
        RHISwapchainAcquireResult AcquireImage();
        const RHIImageView *FetchImageView(uint32_t frameNumber) const;
        bool SubmitFrame(uint32_t frameNumber, const std::vector<RHISyncPoint> &waitSyncPoints);
        void Release();
        ~RHISwapchainImpl();

        const RHISwapchainDesc &GetDesc() const { return m_Desc; }
        RHIFormat GetFormat() const { return m_Format; }
        uint32_t GetImageCount() const { return m_ImageCount; }

        vk::SwapchainKHR GetHandle() const { return m_Swapchain; }
        const std::vector<vk::Image> &GetImages() const { return m_Images; }

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIImageViewImpl<RHIBackend::Vulkan>;

        RHISwapchainImpl(vk::PhysicalDevice physicalDevice,
                         RHIDevice *deviceOwner,
                         const RHISwapchainDesc &desc,
                         uint32_t presentQueueFamilyIndex);

        void ReleaseWithoutUnregister();
        void UnregisterImageView(RHIImageView *view);

        bool m_IsValid = false;
        RHIDevice *m_DeviceOwner = nullptr;
        RHISwapchainDesc m_Desc;
        RHIFormat m_Format = RHIFormat::Undefined;
        uint32_t m_ImageCount = 0;
        vk::Device m_Device;
        vk::SwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<vk::Image> m_Images;
        std::vector<std::unique_ptr<RHIImageView>> m_ImageViews;
        std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
        std::vector<vk::Semaphore> m_PresentSemaphores;
        uint32_t m_NextAcquireSemaphoreIndex = 0;
    };
} // namespace Hazel
