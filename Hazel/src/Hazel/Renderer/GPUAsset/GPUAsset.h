//
// Created by helmholtz on 2026/4/3.
//

#pragma once
#include "Hazel/Asset/Asset.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Core/UUID.h"

namespace Hazel
{
    class Renderer;

    class GPUAsset
    {
    public:
        explicit GPUAsset(UUID uuid,
                          AssetType assetType,
                          Renderer* renderer,
                          uint64_t sourceVersion,
                          uint64_t lastReferencedFrame)
            : m_UUID(uuid), m_Renderer(renderer), m_SourceVersion(sourceVersion),
              m_LastReferencedFrame(lastReferencedFrame),
              m_Type(assetType) {}

        virtual ~GPUAsset() = default;

        uint64_t GetSourceVersion() const { return m_SourceVersion; }
        uint64_t GetLastReferencedFrame() const { return m_LastReferencedFrame; }
        RHISyncPoint* GetLastReferencedSyncPoint() { return &m_LastReferencedSyncPoint; }

        void SetLastReferencedInfo(uint64_t frame, RHISyncPoint syncPoint)
        {
            m_LastReferencedFrame = frame;
            m_LastReferencedSyncPoint = syncPoint;
        }

        void SetLastReferencedFrame(uint64_t frame)
        {
            m_LastReferencedFrame = frame;
            m_LastReferencedSyncPoint.valid = false;
        }

        AssetType GetType() const { return m_Type; }
        UUID GetUUID() const { return m_UUID; }

        int32_t GetUseCount() const { return m_UseCount.load(); }
        bool IsBeingUsed() const { return m_UseCount.load() > 0; }
        void Use() { m_UseCount.fetch_add(1); }
        void Return() { m_UseCount.fetch_sub(1); }

        Renderer* GetRenderer() const { return m_Renderer; }

        virtual void Release() = 0;
        virtual void ReleaseImmediate() = 0;

    protected:
        UUID m_UUID = 0;
        Renderer* m_Renderer = nullptr;
        uint64_t m_SourceVersion = 0;
        RHISyncPoint m_LastReferencedSyncPoint = {};
        uint64_t m_LastReferencedFrame = 0;
        AssetType m_Type = AssetType::Unknown;
        std::atomic<int32_t> m_UseCount = 0;
    };
} // Hazel