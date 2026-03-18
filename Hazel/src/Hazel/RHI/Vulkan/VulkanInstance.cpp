//
// Created by helmholtz on 2026/3/13.
//

#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>

#include "../RHIBase.h"
#include "../RHIInstance.h"
#include "VulkanAdapter.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "VulkanSurface.h"

namespace Hazel
{
    std::vector<RHIAdapter> RHI_VK_FUNC_IMPL(RHIInstance, GetAdapters)()
    {
        const auto physicalDevices = m_Instance.enumeratePhysicalDevices();

        std::vector<RHIAdapter> adapters;
        adapters.reserve(physicalDevices.size());

        for (const auto &physicalDevice: physicalDevices)
        {
            adapters.emplace_back(physicalDevice);
        }

        return adapters;
    }

    RHISurface *RHI_VK_FUNC_IMPL(RHIInstance, CreateSurface)(const RHISurfaceDesc &desc)
    {
        std::unique_ptr<RHISurface> surface(new RHISurface(this, m_Instance, desc));
        if (!surface || !surface->IsValid())
        {
            return nullptr;
        }

        RHISurface *surfacePtr = surface.get();
        RegisterSurface(std::move(surface));
        return surfacePtr;
    }

    RHIDevice *RHI_VK_FUNC_IMPL(RHIInstance, CreateDevice)(const RHIAdapter *adapter,
                                                           const RHIDeviceCapabilities &caps,
                                                           const RHISurface *surface)
    {
        if (!adapter || !adapter->CanCreateDevice(caps))
        {
            return nullptr;
        }

        std::unique_ptr<RHIDevice> device(new RHIDevice(this, m_Instance, *adapter, caps, surface));
        if (!device || !device->IsValid())
        {
            return nullptr;
        }

        RHIDevice *devicePtr = device.get();
        RegisterDevice(std::move(device));
        return devicePtr;
    }

    RHI_VK_FUNC_IMPL(RHIInstance, RHIInstanceImpl)(const RHIInstanceDesc &desc) : m_InstanceDesc(desc)
    {
        vkb::InstanceBuilder builder;
        m_DebugCallbackContext.callback = desc.debugMessageCallback;
        m_DebugCallbackContext.userData = nullptr;

        auto instance = builder.set_app_name(desc.appName.c_str())
                .require_api_version(1, 3, 0)
                .set_app_version(desc.appVersion.major, desc.appVersion.minor, desc.appVersion.patch)
                .set_engine_name(desc.engineName.c_str())
                .set_engine_version(desc.engineVersion.major, desc.engineVersion.minor, desc.engineVersion.patch)
                .enable_validation_layers(desc.useValidation)
                .add_debug_messenger_severity(VulkanConvertDebugMessageSeverity(desc.debugMessageSeverity))
                .add_debug_messenger_type(VulkanConvertDebugMessageType(desc.debugMessageType))
                .set_debug_callback_user_data_pointer(&m_DebugCallbackContext)
                .set_debug_callback(
                    [](VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                       VkDebugUtilsMessageTypeFlagsEXT type,
                       const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                       void *callbackContext) -> VkBool32
                    {
                        if (!callbackContext)
                        {
                            return VK_FALSE;
                        }

                        auto *context = static_cast<VulkanDebugMessageContext *>(callbackContext);
                        DebugMessage message{};
                        message.backend = RHIBackend::Vulkan;
                        message.severity = severity;
                        message.type = type;
                        message.messageIdNumber = callbackData->messageIdNumber;
                        message.messageIdName = callbackData->pMessageIdName;
                        message.message = callbackData->pMessage;
                        context->callback(message, context->userData);
                        return VK_FALSE;
                    })
                .build();

        if (!instance.has_value())
        {
            return;
        }

        m_Instance = instance.value();
        m_DynamicLoader = vk::detail::DispatchLoaderDynamic(m_Instance, vkGetInstanceProcAddr);
        if (desc.useValidation)
        {
            m_DebugMessenger = instance.value().debug_messenger;
        }

        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIInstance, ~RHIInstanceImpl)()
    {
        Release();
    }

    void RHI_VK_FUNC_IMPL(RHIInstance, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        for (const auto &device: m_Devices)
        {
            if (device)
            {
                device->ReleaseFromOwner();
            }
        }
        m_Devices.clear();

        for (const auto &surface: m_Surfaces)
        {
            if (surface)
            {
                surface->ReleaseWithoutUnregister();
            }
        }
        m_Surfaces.clear();

        FlushDeletionQueue();

        if (m_Instance)
        {
            if (m_InstanceDesc.useValidation && m_DebugMessenger)
            {
                m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_DynamicLoader);
            }

            m_Instance.destroy();
        }

        m_DebugMessenger = VK_NULL_HANDLE;
        m_Instance = VK_NULL_HANDLE;
        m_IsValid = false;
    }

    void RHI_VK_FUNC_IMPL(RHIInstance, RegisterDevice)(std::unique_ptr<RHIDevice> device)
    {
        RegisterOwnedObject(m_Devices, std::move(device));
    }

    void RHI_VK_FUNC_IMPL(RHIInstance, UnregisterDevice)(RHIDevice *device)
    {
        UnregisterOwnedObject(m_Devices, device);
    }

    void RHI_VK_FUNC_IMPL(RHIInstance, RegisterSurface)(std::unique_ptr<RHISurface> surface)
    {
        RegisterOwnedObject(m_Surfaces, std::move(surface));
    }

    void RHI_VK_FUNC_IMPL(RHIInstance, UnregisterSurface)(RHISurface *surface)
    {
        UnregisterOwnedObject(m_Surfaces, surface);
    }
} // namespace Hazel
