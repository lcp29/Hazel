// Declares descriptor resource-heap allocation.
// Created: 2026-04-01.

#pragma once

#include "Hazel/RHI/RHI.h"

#include <mutex>
#include <vector>

namespace Hazel
{
    class Renderer;
}

namespace Aster
{

    class ResourceHeapAllocator
    {
      public:
        ResourceHeapAllocator() = delete;
        ResourceHeapAllocator(Hazel::Renderer* renderer);
        ~ResourceHeapAllocator();

        struct HeapRecord
        {
            RHIResourceHeap* heap = nullptr;
            RHIResourceHeapDesc capacity{};
            RHIResourceHeapDesc used{};
            uint32_t usedGroupCount = 0;
            bool isValid = false;
        };

        struct GroupRecord
        {
            RHIResourceGroup* group = nullptr;
            RHIResourceHeap* heap = nullptr;
            RHIResourceHeapDesc usage{};
            bool isValid = false;
        };

        RHIResourceGroup* AllocateGroup(RHIResourceLayout* layout, RHIResourceHeap** outHeap);
        void FreeGroup(RHIResourceGroup* group);
        void Release();

      private:
        Hazel::Renderer* m_Renderer = nullptr;
        bool m_IsValid = false;
        std::mutex m_Mutex;
        std::vector<HeapRecord> m_Heaps;
        std::vector<GroupRecord> m_Groups;

        std::vector<HeapRecord> m_HeapsUpdateAfterBind;
        std::vector<GroupRecord> m_GroupsUpdateAfterBind;
    };
} // namespace Aster
