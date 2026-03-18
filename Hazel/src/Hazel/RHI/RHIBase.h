//
// Created by helmholtz on 2026/3/16.
//

#pragma once

#include <algorithm>
#include <memory>
#include <unordered_set>

namespace Hazel
{
    enum class RHIBackend
    {
        Auto = 1 << 0,
        Vulkan = 1 << 1
    };

    template<typename T>
    using RHIOwnerSet = std::unordered_set<std::unique_ptr<T>>;

    template<typename T>
    T *RegisterOwnedObject(RHIOwnerSet<T> &set, std::unique_ptr<T> object)
    {
        T *ptr = object.get();
        set.insert(std::move(object));
        return ptr;
    }

    template<typename T>
    auto FindOwnedObject(RHIOwnerSet<T> &set, T *object)
    {
        return std::find_if(set.begin(), set.end(), [object](const std::unique_ptr<T> &ownedObject)
        {
            return ownedObject.get() == object;
        });
    }

    template<typename T>
    bool UnregisterOwnedObject(RHIOwnerSet<T> &set, T *object)
    {
        const auto it = FindOwnedObject(set, object);
        if (it == set.end())
        {
            return false;
        }

        set.erase(it);
        return true;
    }
} // namespace Hazel

#define RHI_USE_VULKAN

#ifdef RHI_USE_VULKAN
    #define RHI_BACKEND_API Vulkan
#endif

#define RHI_REGISTER_BASE_CLASS(className) \
    template<Hazel::RHIBackend> \
    class className##Impl {}; \
    using className = className##Impl<Hazel::RHIBackend::RHI_BACKEND_API>;

#define RHI_FORWARD_DECL_CLASS(className) \
    template<> \
    class className##Impl<Hazel::RHIBackend::RHI_BACKEND_API>;

namespace Hazel
{
    RHI_REGISTER_BASE_CLASS(RHIInstance)
    RHI_REGISTER_BASE_CLASS(RHIAdapter)
    RHI_REGISTER_BASE_CLASS(RHISurface)
    RHI_REGISTER_BASE_CLASS(RHISwapchain)
    RHI_REGISTER_BASE_CLASS(RHIDevice)
    RHI_REGISTER_BASE_CLASS(RHIQueue)

    RHI_REGISTER_BASE_CLASS(RHIShader)

    RHI_REGISTER_BASE_CLASS(RHIGraphicsPipeline)
    RHI_REGISTER_BASE_CLASS(RHIComputePipeline)

    RHI_REGISTER_BASE_CLASS(RHICommandPool)
    RHI_REGISTER_BASE_CLASS(RHICommandBuffer)

    RHI_REGISTER_BASE_CLASS(RHIResourceLayout)
    RHI_REGISTER_BASE_CLASS(RHIResourceSignature)
    RHI_REGISTER_BASE_CLASS(RHIResourceHeap)
    RHI_REGISTER_BASE_CLASS(RHIResourceGroup)

    RHI_REGISTER_BASE_CLASS(RHISampler)
    RHI_REGISTER_BASE_CLASS(RHIImage)
    RHI_REGISTER_BASE_CLASS(RHIImageView)

    RHI_REGISTER_BASE_CLASS(RHIBuffer)
    RHI_REGISTER_BASE_CLASS(RHIBufferView)
}


