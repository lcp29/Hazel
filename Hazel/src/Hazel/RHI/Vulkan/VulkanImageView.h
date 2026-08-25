// Declares the Vulkan image view backend.
// Created: 2026-03-15.

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Aster
{
    vk::ImageView CreateImageViewHandle(RHIDevice* device, vk::Image image, const RHIImageViewDesc& desc);

    RHI_VK_CLASS_IMPL(RHIImageView)
    {
      public:
        bool IsValid() const { return m_IsValid; }

        void Release();
        void ReleaseImmediate();

        const RHIImageViewDesc& GetDesc() const { return m_Desc; }

        RHIFormat GetFormat() const { return m_Desc.format; }

        ~RHIImageViewImpl();

        vk::ImageView GetHandle() const { return m_ImageView; }

        RHIImageViewImpl() = default;

      private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIImageImpl<RHIBackend::Vulkan>;
        friend class RHISwapchainImpl<RHIBackend::Vulkan>;

        RHIImageViewImpl(RHIDevice * device, RHIImage * image, const RHIImageViewDesc& desc, bool isDetached);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();

        bool m_IsValid = false;
        RHIImageViewDesc m_Desc;
        RHIImage* m_ImageOwner = nullptr;
        RHISwapchain* m_SwapchainOwner = nullptr;
        RHIDevice* m_DeviceOwner = nullptr;
        vk::ImageView m_ImageView = VK_NULL_HANDLE;
        bool m_IsDetached = false;
    };
} // namespace Aster
