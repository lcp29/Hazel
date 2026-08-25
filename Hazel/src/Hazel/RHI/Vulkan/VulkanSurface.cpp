// Implements the Vulkan surface backend.
// Created: 2026-03-14.

#include "VulkanSurface.h"

#include "VulkanInstance.h"

namespace Aster
{
    RHI_VK_FUNC_IMPL(RHISurface,
                     RHISurfaceImpl)(RHIInstance* instanceOwner, vk::Instance instance, const RHISurfaceDesc& desc)
        : m_InstanceOwner(instanceOwner)
        , m_Desc(desc)
        , m_Instance(instance)
    {
        if (!desc.backendHandle) { return; }

        m_Surface = reinterpret_cast<VkSurfaceKHR>(desc.backendHandle);
        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHISurface, ~RHISurfaceImpl)() { Release(); }

    void RHI_VK_FUNC_IMPL(RHISurface, Release)()
    {
        if (!m_IsValid) { return; }

        auto* instanceOwner = m_InstanceOwner;
        ReleaseWithoutUnregister();

        if (instanceOwner) { instanceOwner->UnregisterSurface(this); }
    }

    void RHI_VK_FUNC_IMPL(RHISurface, ReleaseWithoutUnregister)()
    {
        if (!m_IsValid) { return; }

        const auto instance = m_Instance;
        const auto surface = m_Surface;
        if (m_InstanceOwner)
        {
            m_InstanceOwner->EnqueueDeletion([instance, surface]() {
                if (instance && surface) { instance.destroySurfaceKHR(surface); }
            });
        }
        else if (instance && surface) { instance.destroySurfaceKHR(surface); }

        m_Surface = VK_NULL_HANDLE;
        m_IsValid = false;
        m_InstanceOwner = nullptr;
    }
} // namespace Aster
