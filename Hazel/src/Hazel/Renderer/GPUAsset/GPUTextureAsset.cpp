//
// Created by helmholtz on 2026/3/24.
//

#include "Hazel/Renderer/GPUAsset/GPUTextureAsset.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    GPUTextureAsset::~GPUTextureAsset()
    {
        GPUTextureAsset::ReleaseImmediate();
    }

    void GPUTextureAsset::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_Image->Release();

        m_Image = nullptr;
        m_DefaultImageView = nullptr;
        m_IsValid = false;
    }

    void GPUTextureAsset::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_Image->ReleaseImmediate();

        m_Image = nullptr;
        m_DefaultImageView = nullptr;
        m_IsValid = false;
    }
} // namespace Hazel
