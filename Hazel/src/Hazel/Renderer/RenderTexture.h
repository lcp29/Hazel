//
// Created by helmholtz on 2026/3/22.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class Renderer;

    struct RenderTextureDesc
    {
        uint32_t width = 256;
        uint32_t height = 256;
        uint32_t depth = 1;
        uint32_t arrayLayers = 1;
        RHIImageViewType viewType = Image2D;
        bool useMipmap = false;
        bool perFrame = true;
        RHIFormat format = RHIFormat::BGRA8UNorm;
        RHIImageUsages usages = {};
    };

    class RenderTexture
    {
    public:
        RenderTexture() = delete;
        RenderTexture(UUID uuid, Renderer* renderer, const RenderTextureDesc& desc);

        void Release();
        void ReleaseImmediate();

        // this makes the state of the color image to ShaderRead
        void GenerateMipmap(RHICommandBuffer* commandBuffer);

        RHIImage* GetImage() const;
        const std::vector<RHIImage*>& GetAllImages() const;
        RHIImageView* GetImageView() const;

        const std::vector<RHIImageView*>& GetAllImageViews() const
        {
            return m_ImageViews;
        }

        const RenderTextureDesc& GetDesc() const
        {
            return m_Desc;
        }

        bool IsValid() const
        {
            return m_IsValid;
        }

    private:
        bool m_IsValid = false;
        bool m_PerFrame = true;
        UUID m_UUID = 0;
        uint32_t m_MaxFramesInFlight = 0;
        RenderTextureDesc m_Desc{};
        Renderer* m_Renderer = nullptr;
        std::vector<RHIImage*> m_Images;
        std::vector<RHIImageView*> m_ImageViews;
    };
} // namespace Hazel