// Declares shared RHI ownership and backend-dispatch types.
// Created: 2026-03-16.

#pragma once

#include <algorithm>
#include <memory>

namespace Aster
{
    enum class RHIBackend
    {
        Auto = 1 << 0,
        Vulkan = 1 << 1
    };

    template <typename T> class RHIOwnerSet
    {
      public:
        struct Iterator
        {
            size_t i;
            RHIOwnerSet* owner;

            std::unique_ptr<T>& operator*() { return owner->m_Objects[i]; }

            void skip_front()
            {
                for (; i < owner->m_Objects.size() && !owner->m_Objects[i]; i++)
                    ;
            }

            Iterator& operator++()
            {
                i++;
                skip_front();
                return *this;
            }

            bool operator!=(const Iterator& other) const { return i != other.i || owner != other.owner; }
        };

        T* Register(std::unique_ptr<T> object)
        {
            if (!m_FreeSlots.empty())
            {
                const uint32_t slotIndex = m_FreeSlots.back();
                m_FreeSlots.pop_back();
                m_Objects[slotIndex] = std::move(object);
                return m_Objects[slotIndex].get();
            }
            m_Objects.push_back(std::move(object));
            return m_Objects.back().get();
        }

        uint32_t RegisterHandle(std::unique_ptr<T> object)
        {
            if (!m_FreeSlots.empty())
            {
                uint32_t slotIndex = m_FreeSlots.back();
                m_FreeSlots.pop_back();
                m_Objects[slotIndex] = std::move(object);
                return slotIndex;
            }
            m_Objects.push_back(std::move(object));
            return static_cast<uint32_t>(m_Objects.size() - 1);
        }

        T* Get(uint32_t handle) const
        {
            if (handle >= m_Objects.size()) { return nullptr; }
            return m_Objects[handle].get();
        }

        void Unregister(T* object)
        {
            auto it = std::find_if(m_Objects.begin(), m_Objects.end(), [object](const std::unique_ptr<T>& ownedObject) {
                return ownedObject.get() == object;
            });
            if (it != m_Objects.end())
            {
                it->reset();
                m_FreeSlots.push_back(std::distance(m_Objects.begin(), it));
            }
        }

        void UnregisterHandle(uint32_t handle)
        {
            if (handle >= m_Objects.size()) { return; }

            m_Objects[handle].reset();
            m_FreeSlots.push_back(handle);
        }

        void Clear()
        {
            m_Objects.clear();
            m_FreeSlots.clear();
        }

        Iterator begin()
        {
            Iterator it = {0, this};
            it.skip_front();
            return it;
        }

        Iterator end() { return {m_Objects.size(), this}; }

      private:
        std::vector<std::unique_ptr<T>> m_Objects;
        std::vector<uint32_t> m_FreeSlots;
    };

} // namespace Aster

#ifdef RHI_USE_VULKAN
#define RHI_BACKEND_API Vulkan
#endif

#define RHI_REGISTER_BASE_CLASS(className)                                                                             \
    template <Aster::RHIBackend> class className##Impl                                                                 \
    {};                                                                                                                \
    using className = className##Impl<Aster::RHIBackend::RHI_BACKEND_API>;

#define RHI_FORWARD_DECL_CLASS(className) template <> class className##Impl<Aster::RHIBackend::RHI_BACKEND_API>;

namespace Aster
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
} // namespace Aster
