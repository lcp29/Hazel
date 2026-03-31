//
// Created by helmholtz on 2026/3/25.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class Renderer;

    class Sampler
    {
    public:
        Sampler() = default;

        Sampler(UUID uuid, Renderer* renderer, const RHISamplerDesc& desc, RHISampler* sampler)
            : m_IsValid(true), m_UUID(uuid), m_Desc(desc), m_Renderer(renderer), m_Sampler(sampler) {}

        void Release();
        void ReleaseImmediate();

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
        UUID m_UUID = 0;
        RHISamplerDesc m_Desc{};
        Renderer* m_Renderer = nullptr;
        RHISampler* m_Sampler = nullptr;
    };
} // namespace Hazel