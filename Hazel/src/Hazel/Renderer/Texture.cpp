//
// Created by helmholtz on 2026/3/24.
//

#include "Hazel/Renderer/Texture.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Utils/ImageUtils.h"

namespace Hazel
{
    void Texture::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_Image->Release();

        m_Image = nullptr;
        m_ImageView = nullptr;
        m_IsValid = false;
    }

    void Texture::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_Image->ReleaseImmediate();

        m_Image = nullptr;
        m_ImageView = nullptr;
        m_IsValid = false;
    }

    void Texture::GenerateMipmap(RHICommandBuffer* commandBuffer)
    {
        if (m_Desc.useMipmap)
        {
            ImageUtilGenerateMipmap(commandBuffer, m_Image);
        }
    }
} // namespace Hazel