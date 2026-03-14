//
// Created by helmholtz on 2026/3/13.
//

#include "VulkanInstance.h"

#include <VkBootstrap.h>

#include "VulkanAdapter.h"
#include "VulkanCommon.h"

namespace Hazel
{
    std::vector<Ref<RHIAdapter>> VulkanInstance::GetAdapters()
    {
        auto physicalDevices = m_Instance.enumeratePhysicalDevices();

        std::vector<Ref<RHIAdapter>> adapters;
        adapters.reserve(physicalDevices.size());

        for (const auto &device: physicalDevices)
        {
            adapters.push_back(CreateRef<VulkanAdapter>(device));
        }

        return adapters;
    }

    VulkanInstance::VulkanInstance(const VulkanInstance &&instance) noexcept
    {
        m_IsValid = instance.m_IsValid;
        m_Instance = instance.m_Instance;
        m_InstanceDesc = instance.m_InstanceDesc;
        m_DebugCallbackContext = instance.m_DebugCallbackContext;
    }

    VulkanInstance::VulkanInstance(const RHIInstanceDesc &desc) : m_InstanceDesc(desc)
    {
        vkb::InstanceBuilder builder;
        m_DebugCallbackContext.callback = desc.debugMessageCallback;
        m_DebugCallbackContext.userData = nullptr;
        auto instance = builder.set_app_name(desc.appName.c_str())
                .require_api_version(1, 4, 0)
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
                        if (callbackContext)
                        {
                            VulkanDebugMessageContext *context = static_cast<VulkanDebugMessageContext *>(
                                callbackContext);
                            DebugMessage message{};
                            message.backend = RHIBackend::Vulkan;
                            message.severity = severity;
                            message.type = type;
                            message.messageIdNumber = callbackData->messageIdNumber;
                            message.messageIdName = callbackData->pMessageIdName;
                            message.message = callbackData->pMessage;
                            context->callback(message, context->userData);
                        }
                        return VK_FALSE;
                    })
                .build();

        if (instance.has_value())
        {
            m_Instance = instance.value();
            m_IsValid = true;
            return;
        }
        m_IsValid = false;
    }

    VulkanInstance::~VulkanInstance()
    {
        m_Instance.destroy();
        m_Instance = VK_NULL_HANDLE;
        m_IsValid = false;
    }
} // Hazel
