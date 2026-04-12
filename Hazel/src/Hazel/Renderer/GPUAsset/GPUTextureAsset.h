//
// Created by helmholtz on 2026/3/24.
//

#pragma once

#include "GPUAsset.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

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

    class GPUTextureAsset : public GPUAsset
    {
      public:
        GPUTextureAsset() = delete;

        GPUTextureAsset(const UUID uuid,
                        uint64_t sourceVersion,
                        const TextureDesc& desc,
                        Renderer* renderer,
                        RHIImage* image,
                        RHIImageView* imageView,
                        uint64_t lastReferencedFrame = 0)
            : GPUAsset(uuid, AssetType::Texture, renderer, sourceVersion, lastReferencedFrame)
            , m_IsValid(true)
            , m_Desc(desc)
            , m_Image(image)
            , m_DefaultImageView(imageView)
        {}

        ~GPUTextureAsset() override;

        void Release() override;
        void ReleaseImmediate() override;

        RHIImage* GetImage() const { return m_Image; }

        RHIImageView* GetDefaultImageView() const { return m_DefaultImageView; }

        const TextureDesc& GetDesc() const { return m_Desc; }

        bool IsValid() const { return m_IsValid; }

      private:
        bool m_IsValid = false;
        TextureDesc m_Desc{};
        RHIImage* m_Image = nullptr;
        RHIImageView* m_DefaultImageView = nullptr;
    };
} // namespace Hazel