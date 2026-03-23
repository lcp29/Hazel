//
// Created by helmholtz on 2026/3/14.
//

#include "VulkanSwapchain.h"

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanQueue.h"
#include "VulkanSurface.h"

#include <algorithm>
#include <limits>

namespace Hazel
{
    namespace
    {
        vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats, RHIFormat format)
        {
            const auto requestedFormat = VulkanConvertFormat(format);
            static const std::vector preferredSurfaceFormats = {requestedFormat,
                                                                vk::Format::eB8G8R8A8Unorm,
                                                                vk::Format::eB8G8R8A8Srgb,
                                                                vk::Format::eR8G8B8A8Unorm,
                                                                vk::Format::eR8G8B8A8Srgb};

            for (const auto& preferredFormat : preferredSurfaceFormats)
            {
                for (const auto& surfaceFormat : formats)
                {
                    if (surfaceFormat.format == preferredFormat)
                    {
                        return surfaceFormat;
                    }
                }
            }

            return formats.empty()
                       ? vk::SurfaceFormatKHR(vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear)
                       : formats.front();
        }

        vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes, RHISwapchainMode mode)
        {
            const auto requestedMode = VulkanConvertSwapchainMode(mode);
            for (const auto& presentMode : presentModes)
            {
                if (presentMode == requestedMode)
                {
                    return presentMode;
                }
            }

            return vk::PresentModeKHR::eFifo;
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHISwapchain, RHISwapchainImpl)(vk::PhysicalDevice physicalDevice,
                                                     RHIDevice* deviceOwner,
                                                     const RHISwapchainDesc& desc,
                                                     uint32_t presentQueueFamilyIndex)
        : m_DeviceOwner(deviceOwner),
          m_Desc(desc),
          m_Device(deviceOwner->GetHandle())
    {
        auto* surface = desc.surface;
        if (!surface || !surface->IsValid())
        {
            return;
        }

        const auto surfaceHandle = surface->GetHandle();
        const auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surfaceHandle);
        const auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surfaceHandle);
        const auto presentModes = physicalDevice.getSurfacePresentModesKHR(surfaceHandle);

        if (surfaceFormats.empty() || presentModes.empty())
        {
            return;
        }

        const auto surfaceFormat = ChooseSurfaceFormat(surfaceFormats, desc.format);
        const auto presentMode = ChoosePresentMode(presentModes, desc.mode);
        m_Desc.format = VulkanConvertFormat(surfaceFormat.format);
        m_Desc.mode = VulkanConvertSwapchainMode(presentMode);

        uint32_t minImageCount = desc.imageCount > 0 ? desc.imageCount : surfaceCapabilities.minImageCount;
        minImageCount = std::max(minImageCount, surfaceCapabilities.minImageCount);
        if (surfaceCapabilities.maxImageCount > 0)
        {
            minImageCount = std::min(minImageCount, surfaceCapabilities.maxImageCount);
        }

        vk::Extent2D extent{desc.width, desc.height};
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            extent = surfaceCapabilities.currentExtent;
        }
        else
        {
            extent.width = std::clamp(
                extent.width,
                surfaceCapabilities.minImageExtent.width,
                surfaceCapabilities.maxImageExtent.width);
            extent.height = std::clamp(
                extent.height,
                surfaceCapabilities.minImageExtent.height,
                surfaceCapabilities.maxImageExtent.height);
        }

        auto imageUsage = VulkanConvertImageUsages(desc.usages);
        imageUsage &= surfaceCapabilities.supportedUsageFlags;
        if (imageUsage == vk::ImageUsageFlags())
        {
            imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        }

        const uint32_t queueFamilyIndices[] = {presentQueueFamilyIndex};
        vk::SwapchainCreateInfoKHR createInfo;
        createInfo.surface = surfaceHandle;
        createInfo.minImageCount = minImageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = imageUsage;
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 1;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        m_Swapchain = m_Device.createSwapchainKHR(createInfo);
        if (!m_Swapchain)
        {
            return;
        }

        std::vector<vk::Image> swapchainImages = m_Device.getSwapchainImagesKHR(m_Swapchain);
        m_Format = VulkanConvertFormat(surfaceFormat.format);
        m_ImageCount = static_cast<uint32_t>(swapchainImages.size());
        if (swapchainImages.empty())
        {
            return;
        }

        m_Images.reserve(m_ImageCount);
        m_ImageViews.reserve(m_ImageCount);
        m_ImageAvailableSemaphores.reserve(m_ImageCount);
        m_PresentSemaphores.reserve(m_ImageCount);
        for (uint32_t i = 0; i < m_ImageCount; i++)
        {
            const auto imageAvailableSemaphore = m_Device.createSemaphore({});
            const auto presentSemaphore = m_Device.createSemaphore({});
            if (!imageAvailableSemaphore || !presentSemaphore)
            {
                if (imageAvailableSemaphore)
                {
                    m_Device.destroySemaphore(imageAvailableSemaphore);
                }
                if (presentSemaphore)
                {
                    m_Device.destroySemaphore(presentSemaphore);
                }
                return;
            }

            m_ImageAvailableSemaphores.push_back(imageAvailableSemaphore);
            m_PresentSemaphores.push_back(presentSemaphore);

            RHIImageDesc imageDesc{};
            imageDesc.width = extent.width;
            imageDesc.height = extent.height;
            imageDesc.depth = 1;
            imageDesc.mipLevels = 1;
            imageDesc.arrayLayers = 1;
            imageDesc.format = m_Format;
            imageDesc.usages = m_Desc.usages;
            imageDesc.initialState = RHIImageResourceState::Undefined;

            auto image = std::unique_ptr<RHIImage>(new RHIImage(m_DeviceOwner, imageDesc, swapchainImages[i], true));
            if (!image || !image->IsValid())
            {
                return;
            }

            RHIImageViewDesc viewDesc{};
            viewDesc.viewType = RHIImageViewType::Image2D;
            viewDesc.format = m_Format;
            viewDesc.subresourceRange.planes = RHIImagePlaneFlagBits::Color;
            viewDesc.subresourceRange.levelCount = 1;
            viewDesc.subresourceRange.layerCount = 1;
            viewDesc.componentMapping.r = RHIImageViewComponent::Identity;
            viewDesc.componentMapping.g = RHIImageViewComponent::Identity;
            viewDesc.componentMapping.b = RHIImageViewComponent::Identity;
            viewDesc.componentMapping.a = RHIImageViewComponent::Identity;

            RHIImageView* imageView = image->CreateView(viewDesc);
            if (!imageView)
            {
                return;
            }

            m_ImageViews.push_back(imageView);
            m_Images.push_back(std::move(image));
        }

        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHISwapchain, ~RHISwapchainImpl)()
    {
        Release();
    }

    RHISwapchainAcquireResult RHI_VK_FUNC_IMPL(RHISwapchain, AcquireImage)()
    {
        if (!m_IsValid || !m_DeviceOwner || !m_Swapchain || m_ImageAvailableSemaphores.size() != m_ImageCount)
        {
            return {};
        }

        auto* presentQueue = m_DeviceOwner->GetQueue(RHIQueueType::Present);
        if (!presentQueue || !presentQueue->IsValid())
        {
            return {};
        }

        const uint32_t acquireSemaphoreIndex = m_NextAcquireSemaphoreIndex;
        const auto acquireSemaphore = m_ImageAvailableSemaphores[acquireSemaphoreIndex];
        auto acquireResult = m_Device.acquireNextImageKHR(
            m_Swapchain,
            std::numeric_limits<uint64_t>::max(),
            acquireSemaphore,
            VK_NULL_HANDLE);
        if (acquireResult.result != vk::Result::eSuccess && acquireResult.result != vk::Result::eSuboptimalKHR)
        {
            return {};
        }

        const uint32_t frameNumber = acquireResult.value;
        auto availableSyncPoint = presentQueue->SignalOnBinarySemaphore(acquireSemaphore);
        if (!availableSyncPoint.valid)
        {
            return {};
        }

        m_NextAcquireSemaphoreIndex = (acquireSemaphoreIndex + 1) % m_ImageCount;
        return RHISwapchainAcquireResult{frameNumber, availableSyncPoint};
    }

    RHIImage* RHI_VK_FUNC_IMPL(RHISwapchain, FetchImage)(uint32_t frameNumber) const
    {
        if (!m_IsValid || frameNumber >= m_Images.size())
        {
            return nullptr;
        }
        return m_Images[frameNumber].get();
    }

    RHIImageView* RHI_VK_FUNC_IMPL(RHISwapchain, FetchImageView)(uint32_t frameNumber) const
    {
        if (!m_IsValid || frameNumber >= m_ImageViews.size())
        {
            return nullptr;
        }
        return m_ImageViews[frameNumber];
    }

    bool RHI_VK_FUNC_IMPL(RHISwapchain, SubmitFrame)(uint32_t frameNumber,
                                                     const std::vector<RHISyncPoint>& waitSyncPoints)
    {
        if (!m_IsValid || !m_DeviceOwner || !m_Swapchain || frameNumber >= m_PresentSemaphores.size())
        {
            return false;
        }

        auto* presentQueue = m_DeviceOwner->GetQueue(RHIQueueType::Present);
        if (!presentQueue || !presentQueue->IsValid())
        {
            return false;
        }

        const auto presentSemaphore = m_PresentSemaphores[frameNumber];
        if (!presentQueue->WaitSyncPointsAndSignalBinary(waitSyncPoints, presentSemaphore))
        {
            return false;
        }

        vk::PresentInfoKHR presentInfo;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &presentSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &frameNumber;

        const auto result = presentQueue->GetHandle().presentKHR(presentInfo);
        return result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR;
    }

    void RHI_VK_FUNC_IMPL(RHISwapchain, Release)()
    {
        if (!m_IsValid && !m_Swapchain)
        {
            return;
        }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();

        if (deviceOwner && !m_IsDetached)
        {
            deviceOwner->UnregisterSwapchain(this);
        }
    }

    void RHISwapchainImpl<RHIBackend::Vulkan>::ReleaseImmediate()
    {
        if (!m_IsValid && !m_Swapchain)
        {
            return;
        }

        auto* deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();

        if (deviceOwner && !m_IsDetached)
        {
            deviceOwner->UnregisterSwapchain(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHISwapchain, ReleaseWithoutUnregister)()
    {
        if (!m_IsValid && !m_Swapchain)
        {
            return;
        }

        const auto device = m_Device;
        const auto swapchain = m_Swapchain;
        auto imageAvailableSemaphores = std::move(m_ImageAvailableSemaphores);
        auto presentSemaphores = std::move(m_PresentSemaphores);
        for (auto& image : m_Images)
        {
            if (image)
            {
                image->ReleaseWithoutUnregister();
            }
        }
        m_Images.clear();
        m_ImageViews.clear();

        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device,
                    swapchain,
                    imageAvailableSemaphores = std::move(imageAvailableSemaphores),
                    presentSemaphores = std::move(presentSemaphores)]() {
                    if (device)
                    {
                        for (const auto semaphore : imageAvailableSemaphores)
                        {
                            if (semaphore)
                            {
                                device.destroySemaphore(semaphore);
                            }
                        }
                        for (const auto semaphore : presentSemaphores)
                        {
                            if (semaphore)
                            {
                                device.destroySemaphore(semaphore);
                            }
                        }
                        if (swapchain)
                        {
                            device.destroySwapchainKHR(swapchain);
                        }
                    }
                });
        }
        else if (device)
        {
            for (const auto semaphore : imageAvailableSemaphores)
            {
                if (semaphore)
                {
                    device.destroySemaphore(semaphore);
                }
            }
            for (const auto semaphore : presentSemaphores)
            {
                if (semaphore)
                {
                    device.destroySemaphore(semaphore);
                }
            }
            if (swapchain)
            {
                device.destroySwapchainKHR(swapchain);
            }
        }

        m_Swapchain = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_ImageCount = 0;
        m_Format = RHIFormat::Undefined;
        m_Device = VK_NULL_HANDLE;
        m_NextAcquireSemaphoreIndex = 0;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHISwapchain, ReleaseImmediateWithoutUnregister)()
    {
        if (!m_IsValid && !m_Swapchain)
        {
            return;
        }

        const auto device = m_Device;
        const auto swapchain = m_Swapchain;
        auto imageAvailableSemaphores = std::move(m_ImageAvailableSemaphores);
        auto presentSemaphores = std::move(m_PresentSemaphores);
        for (auto& image : m_Images)
        {
            if (image)
            {
                image->ReleaseImmediateWithoutUnregister();
            }
        }
        m_Images.clear();
        m_ImageViews.clear();

        if (m_DeviceOwner)
        {
            if (device)
            {
                for (const auto semaphore : imageAvailableSemaphores)
                {
                    if (semaphore)
                    {
                        device.destroySemaphore(semaphore);
                    }
                }
                for (const auto semaphore : presentSemaphores)
                {
                    if (semaphore)
                    {
                        device.destroySemaphore(semaphore);
                    }
                }
                if (swapchain)
                {
                    device.destroySwapchainKHR(swapchain);
                }
            }
        }
        else if (device)
        {
            for (const auto semaphore : imageAvailableSemaphores)
            {
                if (semaphore)
                {
                    device.destroySemaphore(semaphore);
                }
            }
            for (const auto semaphore : presentSemaphores)
            {
                if (semaphore)
                {
                    device.destroySemaphore(semaphore);
                }
            }
            if (swapchain)
            {
                device.destroySwapchainKHR(swapchain);
            }
        }

        m_Swapchain = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_ImageCount = 0;
        m_Format = RHIFormat::Undefined;
        m_Device = VK_NULL_HANDLE;
        m_NextAcquireSemaphoreIndex = 0;
        m_IsDetached = false;
    }
} // namespace Hazel