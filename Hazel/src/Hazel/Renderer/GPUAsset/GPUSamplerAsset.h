//
// Created by helmholtz on 2026/3/25.
//

#pragma once

#include "Hazel/Renderer/GPUAsset/GPUAsset.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class Renderer;

    class GPUSamplerAsset : public GPUAsset
    {
    public:
        GPUSamplerAsset() = delete;

        GPUSamplerAsset(UUID uuid,
                        uint64_t sourceVersion,
                        Renderer* renderer,
                        const RHISamplerDesc& desc,
                        RHISampler* sampler,
                        uint64_t lastReferencedFrame = 0)
            : GPUAsset(uuid, AssetType::Sampler, renderer, sourceVersion, lastReferencedFrame),
              m_IsValid(true), m_Desc(desc), m_Sampler(sampler) {}

        void Release() override;
        void ReleaseImmediate() override;

        const RHISamplerDesc& GetDesc() const
        {
            return m_Desc;
        }

        RHISampler* GetHandle() const
        {
            return m_Sampler;
        }

        bool IsValid() const
        {
            return m_IsValid;
        }

    private:
        bool m_IsValid = false;
        RHISamplerDesc m_Desc{};
        RHISampler* m_Sampler = nullptr;
    };
} // namespace Hazel
