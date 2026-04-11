//
// Created by helmholtz on 2026/3/22.
//

#pragma once

#include "Hazel/Renderer/GPUAsset/GPUAsset.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

#include <memory>
#include <vector>

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

    class GPURenderTextureAsset : public GPUAsset
    {
    public:
        GPURenderTextureAsset() = delete;

        GPURenderTextureAsset(UUID uuid,
                              uint64_t sourceVersion,
                              Renderer* renderer,
                              const RenderTextureDesc& desc,
                              std::vector<RHIImage*> images,
                              std::vector<RHIImageView*> imageViews,
                              uint64_t lastReferencedFrame = 0);

        ~GPURenderTextureAsset() override;

        void Release() override;
        void ReleaseImmediate() override;

        void GenerateMipmap(RHICommandBuffer* commandBuffer);

        RHIImage* GetImage() const;
        const std::vector<RHIImage*>& GetAllImages() const;
        RHIImageView* GetDefaultImageView() const;

        const std::vector<RHIImageView*>& GetAllDefaultImageViews() const
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
        uint32_t m_MaxFramesInFlight = 0;
        RenderTextureDesc m_Desc{};
        std::vector<RHIImage*> m_Images;
        std::vector<RHIImageView*> m_ImageViews;
    };

    std::unique_ptr<GPURenderTextureAsset> CreateGPURenderTextureAsset(Renderer* renderer,
                                                                       UUID uuid,
                                                                       uint64_t sourceVersion,
                                                                       const RenderTextureDesc& desc,
                                                                       uint64_t lastReferencedFrame = 0);
} // namespace Hazel