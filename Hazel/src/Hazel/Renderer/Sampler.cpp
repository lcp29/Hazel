//
// Created by helmholtz on 2026/3/25.
//

#include "Hazel/Renderer/Sampler.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    void Sampler::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_Sampler->Release();
        m_Sampler = nullptr;

        m_IsValid = false;
    }

    void Sampler::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_Sampler->ReleaseImmediate();
        m_Sampler = nullptr;

        m_IsValid = false;
    }
} // namespace Hazel
