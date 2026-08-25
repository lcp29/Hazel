// Implements GPU texture asset resources.
// Created: 2026-03-24.

#include "Hazel/Renderer/GPUAsset/GPUTextureAsset.h"

#include "Hazel/Renderer/Renderer.h"

namespace Aster
{
    GPUTextureAsset::~GPUTextureAsset() { GPUTextureAsset::ReleaseImmediate(); }

    void GPUTextureAsset::Release()
    {
        if (!m_IsValid) { return; }

        m_Image->Release();

        m_Image = nullptr;
        m_DefaultImageView = nullptr;
        m_IsValid = false;
    }

    void GPUTextureAsset::ReleaseImmediate()
    {
        if (!m_IsValid) { return; }

        m_Image->ReleaseImmediate();

        m_Image = nullptr;
        m_DefaultImageView = nullptr;
        m_IsValid = false;
    }
} // namespace Aster
