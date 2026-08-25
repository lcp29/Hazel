// Declares the Vulkan surface backend.
// Created: 2026-03-14.

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Aster
{
    RHI_VK_CLASS_IMPL(RHISurface)
    {
      public:
        bool IsValid() const { return m_IsValid; }

        void Release();
        ~RHISurfaceImpl();

        const RHISurfaceDesc& GetDesc() const { return m_Desc; }

        vk::SurfaceKHR GetHandle() const { return m_Surface; }

      private:
        friend class RHIInstanceImpl<RHIBackend::Vulkan>;

        RHISurfaceImpl(RHIInstance * instanceOwner, vk::Instance instance, const RHISurfaceDesc& desc);
        void ReleaseWithoutUnregister();

        bool m_IsValid = false;
        RHIInstance* m_InstanceOwner = nullptr;
        RHISurfaceDesc m_Desc;
        vk::Instance m_Instance;
        vk::SurfaceKHR m_Surface = VK_NULL_HANDLE;
    };
} // namespace Aster
