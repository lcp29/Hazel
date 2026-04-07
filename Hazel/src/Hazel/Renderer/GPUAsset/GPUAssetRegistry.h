//
// Created by helmholtz on 2026/4/3.
//

#pragma once
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Project/Project.h"

#include <memory>
#include <ranges>

namespace Hazel
{
    constexpr int kGarbageCollectionIncrement = 50;

    class GPUAssetRegistry
    {
    public:
        struct ReplaceOrAddResult
        {
            GPUAsset* currentAsset = nullptr;
            std::unique_ptr<GPUAsset> replacedAsset;
        };

        GPUAssetRegistry() = default;

        bool HasAsset(UUID uuid)
        {
            std::unique_lock lock(m_Mutex);
            return m_GPUAssets.contains(uuid);
        }

        void AddAsset(std::unique_ptr<GPUAsset> asset)
        {
            auto uuid = asset->GetUUID();
            auto* assetPointer = asset.get();
            std::unique_lock lock(m_Mutex);
            m_GPUAssets.emplace(uuid, std::move(asset));
            m_GarbageCollectionIterator = m_GPUAssets.begin();
        }

        std::unique_ptr<GPUAsset> RemoveAsset(UUID uuid)
        {
            std::unique_lock lock(m_Mutex);
            auto it = m_GPUAssets.find(uuid);
            if (it != m_GPUAssets.end())
            {
                std::unique_ptr<GPUAsset> asset = std::move(it->second);
                m_GPUAssets.erase(it);
                return asset;
            }
            return nullptr;
        }

        GPUAsset* GetAsset(UUID uuid)
        {
            std::unique_lock lock(m_Mutex);
            auto it = m_GPUAssets.find(uuid);
            if (it != m_GPUAssets.end())
            {
                auto* pointer = it->second.get();
                pointer->Use();
                return it->second.get();
            }
            return nullptr;
        }

        std::unique_ptr<GPUAsset> SetAsset(std::unique_ptr<GPUAsset> asset)
        {
            if (!asset)
            {
                return nullptr;
            }

            auto uuid = asset->GetUUID();
            std::unique_lock lock(m_Mutex);
            auto it = m_GPUAssets.find(uuid);
            if (it != m_GPUAssets.end())
            {
                std::unique_ptr<GPUAsset> oldAsset = std::move(it->second);
                it->second = std::move(asset);
                return oldAsset;
            }

            m_GPUAssets.emplace(uuid, std::move(asset));
            m_GarbageCollectionIterator = m_GPUAssets.begin();
            return nullptr;
        }

        std::pair<std::unique_ptr<GPUAsset>, GPUAsset*> SetAssetAndGetTheOldAndTheNewOnes(
            std::unique_ptr<GPUAsset> asset)
        {
            if (!asset)
            {
                return {nullptr, nullptr};
            }

            auto uuid = asset->GetUUID();
            std::unique_lock lock(m_Mutex);
            auto it = m_GPUAssets.find(uuid);
            if (it != m_GPUAssets.end())
            {
                std::unique_ptr<GPUAsset> oldAsset = std::move(it->second);
                it->second = std::move(asset);
                return {std::move(oldAsset), it->second.get()};
            }

            auto* newPointer = asset.get();
            newPointer->Use();
            m_GPUAssets.emplace(uuid, std::move(asset));
            m_GarbageCollectionIterator = m_GPUAssets.begin();
            return {nullptr, newPointer};
        }

        std::vector<std::unique_ptr<GPUAsset>> AcquireSomeAssetsForGarbageCollection(uint64_t frameUpperBound)
        {
            if (m_GPUAssets.empty())
            {
                return {};
            }

            std::vector<std::unique_ptr<GPUAsset>> unreferencedAssets;

            {
                std::unique_lock lock(m_Mutex);
                int count = 0;
                while (count < kGarbageCollectionIncrement)
                {
                    if (m_GPUAssets.empty())
                    {
                        break;
                    }

                    if (m_GarbageCollectionIterator == m_GPUAssets.end())
                    {
                        m_GarbageCollectionIterator = m_GPUAssets.begin();
                    }

                    auto& asset = m_GarbageCollectionIterator->second;

                    if (asset->GetLastReferencedFrame() < frameUpperBound && !asset->IsBeingUsed())
                    {
                        unreferencedAssets.push_back(std::move(m_GarbageCollectionIterator->second));
                        m_GarbageCollectionIterator = m_GPUAssets.erase(m_GarbageCollectionIterator++);
                    }
                    else
                    {
                        ++m_GarbageCollectionIterator;
                    }

                    count++;
                }
            }

            return unreferencedAssets;
        }

        void EnqueueForDeferredRelease(std::unique_ptr<GPUAsset> asset)
        {
            if (!asset)
            {
                return;
            }

            auto syncPoint = asset->GetLastReferencedSyncPoint();
            if (!syncPoint || syncPoint->valid == false)
            {
                std::unique_lock lock(m_GPUAssetReleaseMutex);
                m_GPUAssetsToRelease.emplace(nullptr, std::move(asset));
                return;
            }
            std::unique_lock lock(m_GPUAssetReleaseMutex);
            m_GPUAssetsToRelease.emplace(syncPoint->queue, std::move(asset));
        }

        void FlushGPUAssetReleaseQueue()
        {
            {
                std::unique_lock lock(m_GPUAssetReleaseMutex);
                std::multimap<RHIQueue*, std::unique_ptr<GPUAsset>> assetsToRelease;
                std::multimap<RHIQueue*, std::unique_ptr<GPUAsset>> assetsNotToRelease;
                for (auto it = m_GPUAssetsToRelease.begin(); it != m_GPUAssetsToRelease.end();)
                {
                    auto [begin, end] = m_GPUAssetsToRelease.equal_range(it->first);
                    if (!it->first)
                    {
                        for (auto iter = begin; iter != end; ++iter)
                        {
                            assetsToRelease.emplace(iter->first, std::move(iter->second));
                        }
                    }
                    else
                    {
                        for (auto iter = begin; iter != end; ++iter)
                        {
                            auto syncPoint = iter->second->GetLastReferencedSyncPoint();
                            if (!syncPoint || syncPoint->valid == false)
                            {
                                assetsToRelease.emplace(iter->first, std::move(iter->second));
                            }
                            else
                            {
                                auto currentTimelineValue = iter->first->GetCurrentTimelineValue();
                                if (syncPoint->value <= currentTimelineValue)
                                {
                                    assetsToRelease.emplace(iter->first, std::move(iter->second));
                                }
                                else
                                {
                                    assetsNotToRelease.emplace(iter->first, std::move(iter->second));
                                }
                            }
                        }
                    }
                    it = end;
                }
                m_GPUAssetsToRelease = std::move(assetsNotToRelease);
                std::thread([assetsToRelease = std::move(assetsToRelease)]() mutable {
                    for (auto& asset : assetsToRelease | std::views::values)
                    {
                        asset.reset();
                    }
                }).detach();
            }
        }

        std::mutex& GetPipelineCacheMutex()
        {
            return m_PipelineCacheMutex;
        }

    private:
        using GPUAssetMap = std::unordered_map<UUID, std::unique_ptr<GPUAsset>>;

        std::mutex m_PipelineCacheMutex;
        std::mutex m_Mutex;
        GPUAssetMap m_GPUAssets;
        GPUAssetMap::iterator m_GarbageCollectionIterator;
        std::mutex m_GPUAssetReleaseMutex;
        std::multimap<RHIQueue*, std::unique_ptr<GPUAsset>> m_GPUAssetsToRelease;
    };
}