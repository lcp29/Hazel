//
// Created by helmholtz on 2026/3/24.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

#include <filesystem>

namespace Hazel
{
    class Renderer;

    struct TextureDesc
    {
        uint32_t width = 256;
        uint32_t height = 256;
        bool useMipmap = false;
        RHIFormat format = RHIFormat::BGRA8UNorm;
        RHIImageUsages usages = {};
    };

    class Texture
    {
    public:
        Texture() = default;

        Texture(const UUID uuid, const TextureDesc& desc, Renderer* renderer, RHIImage* image, RHIImageView* imageView)
            : m_IsValid(true), m_UUID(uuid), m_Desc(desc), m_Renderer(renderer), m_Image(image),
              m_ImageView(imageView) {}

        void Release();
        void ReleaseImmediate();

        void GenerateMipmap(RHICommandBuffer* commandBuffer);

        RHIImage* GetImage() const
        {
            return m_Image;
        }

        RHIImageView* GetImageView() const
        {
            return m_ImageView;
        }

        const TextureDesc& GetDesc() const
        {
            return m_Desc;
        }

        bool IsValid() const
        {
            return m_IsValid;
        }

    private:
        bool m_IsValid = false;
        UUID m_UUID = 0;
        TextureDesc m_Desc{};
        Renderer* m_Renderer = nullptr;
        RHIImage* m_Image = nullptr;
        RHIImageView* m_ImageView = nullptr;
    };
} // namespace Hazel