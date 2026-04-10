//
// Created by helmholtz on 2026/4/1.
//

#include "ResourceHeapAllocator.h"

#include "Hazel/Renderer/Renderer.h"

#include <algorithm>

namespace Hazel
{
    namespace
    {
        constexpr uint32_t kHeapMaxGroups = 64;
        constexpr uint32_t kMinDescriptorCapacity = 64;

        RHIResourceHeapDesc GetGroupUsage(RHIResourceLayout* layout)
        {
            RHIResourceHeapDesc usage{};
            usage.maxGroups = 1;
            usage.updateAfterBind = false;

            for (const auto& binding : layout->GetDesc().bindings)
            {
                if (binding.updateAfterBind)
                {
                    usage.updateAfterBind = true;
                }

                switch (binding.type)
                {
                    case RHIResourceBindingType::Sampler:
                        usage.samplerCount += binding.count;
                        break;
                    case RHIResourceBindingType::SamplerWithImage:
                        usage.samplerWithImageCount += binding.count;
                        break;
                    case RHIResourceBindingType::SampledImage:
                        usage.sampledImageCount += binding.count;
                        break;
                    case RHIResourceBindingType::StorageImage:
                        usage.storageImageCount += binding.count;
                        break;
                    case RHIResourceBindingType::UniformBuffer:
                    case RHIResourceBindingType::UniformDynamicBuffer:
                        usage.uniformBufferCount += binding.count;
                        break;
                    case RHIResourceBindingType::StorageBuffer:
                    case RHIResourceBindingType::StorageDynamicBuffer:
                        usage.storageBufferCount += binding.count;
                        break;
                    case RHIResourceBindingType::UniformTexelBuffer:
                        usage.uniformTexelBufferCount += binding.count;
                        break;
                    case RHIResourceBindingType::StorageTexelBuffer:
                        usage.storageTexelBufferCount += binding.count;
                        break;
                }
            }

            return usage;
        }

        bool CanAllocate(const ResourceHeapAllocator::HeapRecord& heapRecord, const RHIResourceHeapDesc& usage)
        {
            return heapRecord.usedGroupCount + usage.maxGroups + 1 <= heapRecord.capacity.maxGroups &&
                   heapRecord.used.samplerCount + usage.samplerCount + 1 <= heapRecord.capacity.samplerCount &&
                   heapRecord.used.samplerWithImageCount + usage.samplerWithImageCount + 1 <= heapRecord.capacity.
                   samplerWithImageCount &&
                   heapRecord.used.sampledImageCount + usage.sampledImageCount + 1 <= heapRecord.capacity.sampledImageCount
                   &&
                   heapRecord.used.storageImageCount + usage.storageImageCount + 1 <= heapRecord.capacity.storageImageCount
                   &&
                   heapRecord.used.uniformBufferCount + usage.uniformBufferCount + 1 <= heapRecord.capacity.
                   uniformBufferCount &&
                   heapRecord.used.storageBufferCount + usage.storageBufferCount + 1 <= heapRecord.capacity.
                   storageBufferCount &&
                   heapRecord.used.uniformTexelBufferCount + usage.uniformTexelBufferCount + 1 <= heapRecord.capacity.
                   uniformTexelBufferCount &&
                   heapRecord.used.storageTexelBufferCount + usage.storageTexelBufferCount + 1 <= heapRecord.capacity.
                   storageTexelBufferCount;
        }

        void AddUsage(RHIResourceHeapDesc& target, const RHIResourceHeapDesc& usage)
        {
            target.maxGroups += usage.maxGroups;
            target.samplerCount += usage.samplerCount;
            target.samplerWithImageCount += usage.samplerWithImageCount;
            target.sampledImageCount += usage.sampledImageCount;
            target.storageImageCount += usage.storageImageCount;
            target.uniformBufferCount += usage.uniformBufferCount;
            target.storageBufferCount += usage.storageBufferCount;
            target.uniformTexelBufferCount += usage.uniformTexelBufferCount;
            target.storageTexelBufferCount += usage.storageTexelBufferCount;
        }

        void RemoveUsage(RHIResourceHeapDesc& target, const RHIResourceHeapDesc& usage)
        {
            target.maxGroups -= usage.maxGroups;
            target.samplerCount -= usage.samplerCount;
            target.samplerWithImageCount -= usage.samplerWithImageCount;
            target.sampledImageCount -= usage.sampledImageCount;
            target.storageImageCount -= usage.storageImageCount;
            target.uniformBufferCount -= usage.uniformBufferCount;
            target.storageBufferCount -= usage.storageBufferCount;
            target.uniformTexelBufferCount -= usage.uniformTexelBufferCount;
            target.storageTexelBufferCount -= usage.storageTexelBufferCount;
        }

        uint32_t GetCapacityCount(uint32_t usageCount)
        {
            return usageCount == 0 ? 0 : std::max(usageCount * kHeapMaxGroups, kMinDescriptorCapacity);
        }

        RHIResourceHeapDesc GetHeapCapacity(const RHIResourceHeapDesc& usage)
        {
            RHIResourceHeapDesc capacity{};
            capacity.maxGroups = kHeapMaxGroups;
            capacity.samplerCount = GetCapacityCount(usage.samplerCount);
            capacity.samplerWithImageCount = GetCapacityCount(usage.samplerWithImageCount);
            capacity.sampledImageCount = GetCapacityCount(usage.sampledImageCount);
            capacity.storageImageCount = GetCapacityCount(usage.storageImageCount);
            capacity.uniformBufferCount = GetCapacityCount(usage.uniformBufferCount);
            capacity.storageBufferCount = GetCapacityCount(usage.storageBufferCount);
            capacity.uniformTexelBufferCount = GetCapacityCount(usage.uniformTexelBufferCount);
            capacity.storageTexelBufferCount = GetCapacityCount(usage.storageTexelBufferCount);
            capacity.updateAfterBind = usage.updateAfterBind;
            return capacity;
        }
    } // namespace

    ResourceHeapAllocator::ResourceHeapAllocator(Renderer* renderer)
        : m_Renderer(renderer), m_IsValid(true) {}

    ResourceHeapAllocator::~ResourceHeapAllocator()
    {
        Release();
    }

    RHIResourceGroup* ResourceHeapAllocator::AllocateGroup(RHIResourceLayout* layout, RHIResourceHeap** outHeap)
    {
        auto usage = GetGroupUsage(layout);

        auto& heaps = usage.updateAfterBind ? m_HeapsUpdateAfterBind : m_Heaps;
        auto& groups = usage.updateAfterBind ? m_GroupsUpdateAfterBind : m_Groups;

        for (auto& heapRecord : heaps)
        {
            if (!heapRecord.isValid)
            {
                continue;
            }
            if (!CanAllocate(heapRecord, usage))
            {
                continue;
            }

            auto* group = heapRecord.heap->CreateGroup(layout);
            if (group)
            {
                AddUsage(heapRecord.used, usage);
                heapRecord.usedGroupCount += usage.maxGroups;

                GroupRecord groupRecord{};
                groupRecord.group = group;
                groupRecord.heap = heapRecord.heap;
                groupRecord.usage = usage;
                groupRecord.isValid = true;
                groups.push_back(groupRecord);
            }
            if (outHeap)
            {
                *outHeap = heapRecord.heap;
            }
            return group;
        }

        auto capacity = GetHeapCapacity(usage);
        auto* heap = m_Renderer->GetDevice()->CreateResourceHeap(capacity);
        auto* group = heap->CreateGroup(layout);

        HeapRecord heapRecord{};
        heapRecord.heap = heap;
        heapRecord.capacity = capacity;
        heapRecord.used = {};
        if (group)
        {
            AddUsage(heapRecord.used, usage);
        }
        heapRecord.usedGroupCount = group ? usage.maxGroups : 0;
        heapRecord.isValid = true;
        heaps.push_back(heapRecord);

        if (group)
        {
            GroupRecord groupRecord{};
            groupRecord.group = group;
            groupRecord.heap = heap;
            groupRecord.usage = usage;
            groupRecord.isValid = true;
            groups.push_back(groupRecord);
        }

        if (outHeap)
        {
            *outHeap = heap;
        }
        return group;
    }

    void ResourceHeapAllocator::FreeGroup(RHIResourceGroup* group)
    {
        auto usage = GetGroupUsage(group->GetLayout());
        auto& heaps = usage.updateAfterBind ? m_HeapsUpdateAfterBind : m_Heaps;
        auto& groups = usage.updateAfterBind ? m_GroupsUpdateAfterBind : m_Groups;

        auto groupIt = std::ranges::find_if(groups,
                                            [group](const GroupRecord& groupRecord) {
                                                return groupRecord.isValid && groupRecord.group == group;
                                            });
        if (groupIt == groups.end())
        {
            return;
        }

        groupIt->group->Release();

        auto heapIt = std::ranges::find_if(heaps,
                                           [groupIt](const HeapRecord& heapRecord) {
                                               return heapRecord.isValid && heapRecord.heap == groupIt->heap;
                                           });
        if (heapIt != heaps.end())
        {
            RemoveUsage(heapIt->used, groupIt->usage);
            heapIt->usedGroupCount -= groupIt->usage.maxGroups;
        }

        groupIt->group = nullptr;
        groupIt->heap = nullptr;
        groupIt->usage = {};
        groupIt->isValid = false;
    }

    void ResourceHeapAllocator::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        for (auto& heapRecord : m_Heaps)
        {
            if (!heapRecord.isValid)
            {
                continue;
            }
            if (heapRecord.heap)
            {
                heapRecord.heap->Release();
            }
            heapRecord.heap = nullptr;
            heapRecord.capacity = {};
            heapRecord.used = {};
            heapRecord.usedGroupCount = 0;
            heapRecord.isValid = false;
        }

        for (auto& heapRecord : m_HeapsUpdateAfterBind)
        {
            if (!heapRecord.isValid)
            {
                continue;
            }
            if (heapRecord.heap)
            {
                heapRecord.heap->Release();
            }
            heapRecord.heap = nullptr;
            heapRecord.capacity = {};
            heapRecord.used = {};
            heapRecord.usedGroupCount = 0;
            heapRecord.isValid = false;
        }

        m_Heaps.clear();
        m_Groups.clear();
        m_HeapsUpdateAfterBind.clear();
        m_GroupsUpdateAfterBind.clear();
        m_IsValid = false;
    }
} // namespace Hazel