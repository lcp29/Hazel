//
// Created by helmholtz on 2026/3/15.
//

#pragma once

#include "../RHIHeaders.h"
#include "VulkanBase.h"

#include <vulkan/vulkan.hpp>

namespace Hazel
{
    RHI_VK_CLASS_IMPL(RHIResourceHeap)
    {
    public:
        bool IsValid() const { return m_IsValid; }
        RHIResourceGroup *CreateGroup(RHIResourceLayout *layout);
        void Release();
        void ReleaseImmediate();
        ~RHIResourceHeapImpl();

        const RHIResourceHeapDesc &GetDesc() const { return m_Desc; }
        vk::DescriptorPool GetHandle() const { return m_DescriptorPool; }
        void EnqueueDeletion(DeletionQueue::Operation operation) { m_DeletionQueue.Enqueue(std::move(operation)); }

    private:
        friend class RHIDeviceImpl<RHIBackend::Vulkan>;
        friend class RHIResourceGroupImpl<RHIBackend::Vulkan>;

        RHIResourceHeapImpl(RHIDevice *deviceOwner, vk::Device device, const RHIResourceHeapDesc &desc);

        void ReleaseWithoutUnregister();
        void ReleaseImmediateWithoutUnregister();
        void RegisterGroup(std::unique_ptr<RHIResourceGroup> group);
        void UnregisterGroup(RHIResourceGroup *group);

        bool m_IsValid = false;
        RHIResourceHeapDesc m_Desc;
        RHIDevice *m_DeviceOwner = nullptr;
        vk::Device m_Device;
        vk::DescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        DeletionQueue m_DeletionQueue;
        RHIOwnerSet<RHIResourceGroup> m_Groups;
    };
} // Hazel
