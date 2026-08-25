// Implements GPU sampler asset resources.
// Created: 2026-03-25.

#include "Hazel/Renderer/GPUAsset/GPUSamplerAsset.h"

#include "Hazel/Renderer/Renderer.h"

namespace Aster
{
    GPUSamplerAsset::~GPUSamplerAsset() { GPUSamplerAsset::ReleaseImmediate(); }

    void GPUSamplerAsset::Release()
    {
        if (!m_IsValid) { return; }

        m_Sampler->Release();
        m_Sampler = nullptr;

        m_IsValid = false;
    }

    void GPUSamplerAsset::ReleaseImmediate()
    {
        if (!m_IsValid) { return; }

        m_Sampler->ReleaseImmediate();
        m_Sampler = nullptr;

        m_IsValid = false;
    }
} // namespace Aster
