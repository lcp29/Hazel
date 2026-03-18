//
// Created by helmholtz on 2026/3/14.
//

#include "VulkanSwapchain.h"

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanImageView.h"
#include "VulkanQueue.h"
#include "VulkanSurface.h"

#include <algorithm>
#include <limits>

namespace Hazel
{
    namespace
    {
        vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats, RHIFormat format)
        {
            const auto requestedFormat = VulkanConvertFormat(format);
            for (const auto &surfaceFormat: formats)
            {
                if (surfaceFormat.format == requestedFormat)
                {
                    return surfaceFormat;
                }
            }

            return formats.empty() ? vk::SurfaceFormatKHR(vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear)
                                   : formats.front();
        }

        vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR> &presentModes,
                                             RHISwapchainMode mode)
        {
            const auto requestedMode = VulkanConvertSwapchainMode(mode);
            for (const auto &presentMode: presentModes)
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
                                                     RHIDevice *deviceOwner,
                                                     const RHISwapchainDesc &desc,
                                                     uint32_t presentQueueFamilyIndex)
        : m_Desc(desc), m_Device(deviceOwner->GetHandle()), m_DeviceOwner(deviceOwner)
    {
        auto *surface = desc.surface;
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
            extent.width = std::clamp(extent.width,
                                      surfaceCapabilities.minImageExtent.width,
                                      surfaceCapabilities.maxImageExtent.width);
            extent.height = std::clamp(extent.height,
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

        m_Images = m_Device.getSwapchainImagesKHR(m_Swapchain);
        m_Format = VulkanConvertFormat(surfaceFormat.format);
        m_ImageCount = static_cast<uint32_t>(m_Images.size());
        if (m_Images.empty())
        {
            return;
        }

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

            m_ImageViews.push_back(std::make_unique<RHIImageView>());
            auto imageView = m_ImageViews.back().get();
            imageView->m_DeviceOwner = m_DeviceOwner;
            imageView->m_SwapchainOwner = this;
            imageView->m_ImageOwner = nullptr;
            imageView->m_Desc.viewType = RHIImageViewType::Image2D;
            imageView->m_Desc.format = m_Format;
            imageView->m_Desc.subresourceRange.planes = RHIImagePlaneFlagBits::Color;
            imageView->m_Desc.subresourceRange.levelCount = 1;
            imageView->m_Desc.subresourceRange.layerCount = 1;
            imageView->m_Desc.componentMapping.r = RHIImageViewComponent::Identity;
            imageView->m_Desc.componentMapping.g = RHIImageViewComponent::Identity;
            imageView->m_Desc.componentMapping.b = RHIImageViewComponent::Identity;
            imageView->m_Desc.componentMapping.a = RHIImageViewComponent::Identity;
            imageView->m_ImageView = CreateImageViewHandle(m_DeviceOwner, m_Images[i], imageView->m_Desc);
            if (!imageView->m_ImageView)
            {
                return;
            }
            imageView->m_IsValid = true;
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

        auto *presentQueue = m_DeviceOwner->GetQueue(RHIQueueType::Present);
        if (!presentQueue || !presentQueue->IsValid())
        {
            return {};
        }

        const uint32_t acquireSemaphoreIndex = m_NextAcquireSemaphoreIndex;
        const auto acquireSemaphore = m_ImageAvailableSemaphores[acquireSemaphoreIndex];
        auto acquireResult = m_Device.acquireNextImageKHR(m_Swapchain,
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
        return RHISwapchainAcquireResult{
            frameNumber,
            availableSyncPoint
        };
    }

    const RHIImageView *RHI_VK_FUNC_IMPL(RHISwapchain, FetchImageView)(uint32_t frameNumber) const
    {
        if (!m_IsValid || frameNumber >= m_ImageViews.size())
        {
            return nullptr;
        }
        return m_ImageViews[frameNumber].get();
    }

    bool RHI_VK_FUNC_IMPL(RHISwapchain, SubmitFrame)(uint32_t frameNumber, const std::vector<RHISyncPoint> &waitSyncPoints)
    {
        if (!m_IsValid || !m_DeviceOwner || !m_Swapchain || frameNumber >= m_PresentSemaphores.size())
        {
            return false;
        }

        auto *presentQueue = m_DeviceOwner->GetQueue(RHIQueueType::Present);
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

        for (auto &imageView : m_ImageViews)
        {
            if (imageView)
            {
                imageView->ReleaseWithoutUnregister();
            }
        }
        m_ImageViews.clear();

        auto *deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();

        if (deviceOwner)
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

        for (const auto &imageView: m_ImageViews)
        {
            if (imageView)
            {
                imageView->ReleaseWithoutUnregister();
            }
        }
        m_ImageViews.clear();

        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device,
                                            swapchain,
                                            imageAvailableSemaphores = std::move(imageAvailableSemaphores),
                                            presentSemaphores = std::move(presentSemaphores)]()
            {
                if (device)
                {
                    for (const auto semaphore: imageAvailableSemaphores)
                    {
                        if (semaphore)
                        {
                            device.destroySemaphore(semaphore);
                        }
                    }
                    for (const auto semaphore: presentSemaphores)
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
            for (const auto semaphore: imageAvailableSemaphores)
            {
                if (semaphore)
                {
                    device.destroySemaphore(semaphore);
                }
            }
            for (const auto semaphore: presentSemaphores)
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

        m_Images.clear();
        m_Swapchain = VK_NULL_HANDLE;
        m_IsValid = false;
        m_DeviceOwner = nullptr;
        m_ImageCount = 0;
        m_Format = RHIFormat::Undefined;
        m_Device = VK_NULL_HANDLE;
        m_NextAcquireSemaphoreIndex = 0;
    }

    void RHI_VK_FUNC_IMPL(RHISwapchain, UnregisterImageView)(RHIImageView *view)
    {
        std::erase_if(m_ImageViews, [view](const std::unique_ptr<RHIImageView> &imageView)
        {
            return imageView.get() == view;
        });
    }
} // namespace Hazel
