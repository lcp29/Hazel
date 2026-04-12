//
// Created by helmholtz on 2026/3/15.
//

#include "VulkanImageView.h"

#include "../RHIImageView.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanSwapchain.h"

namespace Hazel
{
    vk::ImageView CreateImageViewHandle(RHIDevice* device, vk::Image image, const RHIImageViewDesc& desc)
    {
        if (!device || !image || desc.format == RHIFormat::Undefined) { return VK_NULL_HANDLE; }

        vk::ImageViewCreateInfo createInfo{};
        createInfo.image = image;
        createInfo.format = VulkanConvertFormat(desc.format);
        createInfo.components = VulkanConvertImageViewComponentMapping(desc.componentMapping);
        createInfo.viewType = VulkanConvertImageViewType(desc.viewType);
        createInfo.subresourceRange.aspectMask = VulkanConvertImagePlanes(desc.subresourceRange.planes);
        createInfo.subresourceRange.baseMipLevel = desc.subresourceRange.baseMipLevel;
        createInfo.subresourceRange.levelCount = desc.subresourceRange.levelCount;
        createInfo.subresourceRange.baseArrayLayer = desc.subresourceRange.baseArrayLayer;
        createInfo.subresourceRange.layerCount = desc.subresourceRange.layerCount;

        vk::ImageView imageView = VK_NULL_HANDLE;
        const auto result = device->GetHandle().createImageView(&createInfo, nullptr, &imageView);
        return result == vk::Result::eSuccess ? imageView : VK_NULL_HANDLE;
    }

    RHI_VK_FUNC_IMPL(RHIImageView, RHIImageViewImpl)(RHIDevice* device,
                                                     RHIImage* image,
                                                     const RHIImageViewDesc& desc,
                                                     bool isDetached)
    {
        if (!device || !image || desc.format == RHIFormat::Undefined) { return; }

        HZ_RHI_DEBUG_RETURN_IF(!image->IsValid());

        m_ImageOwner = image;
        m_DeviceOwner = device;
        m_Desc = desc;
        m_IsDetached = isDetached;

        m_ImageView = CreateImageViewHandle(device, image->GetHandle(), desc);
        if (!m_ImageView) { return; }

        if (!m_IsDetached)
        {
            std::unique_ptr<RHIImageView> self(this);
            image->RegisterView(std::move(self));
        }
        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIImageView, ~RHIImageViewImpl)() { Release(); }

    void RHI_VK_FUNC_IMPL(RHIImageView, Release)()
    {
        if (!m_IsValid) { return; }

        auto* imageOwner = m_ImageOwner;
        ReleaseWithoutUnregister();
        if (imageOwner && !m_IsDetached) { imageOwner->UnregisterView(this); }
    }

    void RHI_VK_FUNC_IMPL(RHIImageView, ReleaseImmediate)()
    {
        if (!m_IsValid) { return; }

        auto* imageOwner = m_ImageOwner;
        ReleaseImmediateWithoutUnregister();
        if (imageOwner && !m_IsDetached) { imageOwner->UnregisterView(this); }
    }

    void RHI_VK_FUNC_IMPL(RHIImageView, ReleaseWithoutUnregister)()
    {
        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device = m_DeviceOwner->GetHandle(), imageView = m_ImageView]() {
                device.destroyImageView(imageView);
            });
        }

        m_ImageOwner = nullptr;
        m_SwapchainOwner = nullptr;
        m_DeviceOwner = nullptr;
        m_ImageView = VK_NULL_HANDLE;
        m_IsValid = false;
        m_IsDetached = false;
    }

    void RHI_VK_FUNC_IMPL(RHIImageView, ReleaseImmediateWithoutUnregister)()
    {
        if (m_DeviceOwner) { m_DeviceOwner->GetHandle().destroyImageView(m_ImageView); }

        m_ImageOwner = nullptr;
        m_SwapchainOwner = nullptr;
        m_DeviceOwner = nullptr;
        m_ImageView = VK_NULL_HANDLE;
        m_IsValid = false;
        m_IsDetached = false;
    }
} // namespace Hazel