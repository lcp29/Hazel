// Declares GPU sampler asset resources.
// Created: 2026-03-25.

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"

namespace Hazel
{
    class Renderer;
}

namespace Aster
{

    class GPUSamplerAsset : public GPUAsset
    {
      public:
        GPUSamplerAsset() = delete;

        GPUSamplerAsset(Hazel::UUID uuid,
                        uint64_t sourceVersion,
                        Hazel::Renderer* renderer,
                        const RHISamplerDesc& desc,
                        RHISampler* sampler,
                        uint64_t lastReferencedFrame = 0)
            : GPUAsset(uuid, AssetType::Sampler, renderer, sourceVersion, lastReferencedFrame)
            , m_IsValid(true)
            , m_Desc(desc)
            , m_Sampler(sampler)
        {}

        ~GPUSamplerAsset() override;

        void Release() override;
        void ReleaseImmediate() override;

        const RHISamplerDesc& GetDesc() const { return m_Desc; }

        RHISampler* GetHandle() const { return m_Sampler; }

        bool IsValid() const { return m_IsValid; }

      private:
        bool m_IsValid = false;
        RHISamplerDesc m_Desc{};
        RHISampler* m_Sampler = nullptr;
    };
} // namespace Aster
