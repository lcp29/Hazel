//
// Created by helmholtz on 2026/3/22.
//

#include "Hazel/Renderer/GPUAsset/GPURenderTextureAsset.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Utils/ImageUtils.h"

#include <unordered_set>

namespace Hazel
{
    namespace
    {
        RHIImagePlanes GetImagePlanesFromFormat(RHIFormat format)
        {
            static const std::unordered_set depthFormats =
                {RHIFormat::D32SFloat, RHIFormat::D32SFloatS8Uint};
            static const std::unordered_set stencilFormats =
                {RHIFormat::D32SFloatS8Uint, RHIFormat::S8Uint};
            static const std::unordered_set colorFormats =
            {
                RHIFormat::R8UNorm,
                RHIFormat::R32SInt,
                RHIFormat::RG8UNorm,
                RHIFormat::R32SFloat,
                RHIFormat::RG32SFloat,
                RHIFormat::RGB32SFloat,
                RHIFormat::RG16UNorm,
                RHIFormat::BGRA8UNorm,
                RHIFormat::BGRA8SRGB,
                RHIFormat::RGBA8UNorm,
                RHIFormat::RGBA8SRGB,
                RHIFormat::RGB10A2UNorm,
                RHIFormat::RGBA16SFloat,
            };
            RHIImagePlanes imagePlane;
            if (depthFormats.contains(format))
            {
                imagePlane |= RHIImagePlaneFlagBits::Depth;
            }
            if (stencilFormats.contains(format))
            {
                imagePlane |= RHIImagePlaneFlagBits::Stencil;
            }
            if (colorFormats.contains(format))
            {
                imagePlane |= RHIImagePlaneFlagBits::Color;
            }
            return imagePlane;
        }
    }

    std::unique_ptr<GPURenderTextureAsset> CreateGPURenderTextureAsset(Renderer* renderer,
                                                                       UUID uuid,
                                                                       uint64_t sourceVersion,
                                                                       const RenderTextureDesc& desc,
                                                                       uint64_t lastReferencedFrame)
    {
        auto* device = renderer->GetDevice();

        RenderTextureDesc resolvedDesc = desc;

        RHIImageDesc imageDesc{};
        imageDesc.width = resolvedDesc.width;
        imageDesc.height = resolvedDesc.height;
        imageDesc.depth = resolvedDesc.depth;
        imageDesc.mipLevels = resolvedDesc.useMipmap ? DeduceMipLevelCount(resolvedDesc.width, resolvedDesc.height) : 1;
        imageDesc.arrayLayers = resolvedDesc.arrayLayers;
        imageDesc.format = resolvedDesc.format;
        imageDesc.usages = RHIImageUsageFlagBits::Sampled | resolvedDesc.usages;

        auto imagePlanes = GetImagePlanesFromFormat(resolvedDesc.format);

        if (imagePlanes & RHIImagePlaneFlagBits::Depth || imagePlanes & RHIImagePlaneFlagBits::Stencil)
        {
            imageDesc.usages |= RHIImageUsageFlagBits::DepthStencilAttachment;
        }
        else
        {
            imageDesc.usages |= RHIImageUsageFlagBits::ColorAttachment;
        }

        if (resolvedDesc.useMipmap)
        {
            imageDesc.usages |= RHIImageUsageFlagBits::TransferSource | RHIImageUsageFlagBits::TransferDestination;
        }

        resolvedDesc.usages = imageDesc.usages;

        RHIImageViewDesc viewDesc{};
        viewDesc.viewType = resolvedDesc.viewType;
        viewDesc.format = imageDesc.format;
        viewDesc.subresourceRange.baseMipLevel = 0;
        viewDesc.subresourceRange.levelCount = imageDesc.mipLevels;
        viewDesc.subresourceRange.baseArrayLayer = 0;
        viewDesc.subresourceRange.layerCount = resolvedDesc.arrayLayers;
        viewDesc.subresourceRange.planes = imagePlanes;
        imageDesc.initialState = RHIImageResourceState::Undefined;

        std::vector<RHIImage*> images;
        std::vector<RHIImageView*> imageViews;

        if (resolvedDesc.perFrame)
        {
            images.resize(renderer->GetMaxFramesInFlight(), nullptr);
            imageViews.resize(renderer->GetMaxFramesInFlight(), nullptr);

            for (int i = 0; i < renderer->GetMaxFramesInFlight(); i++)
            {
                auto* image = device->CreateImage(imageDesc);
                auto* imageView = image->CreateView(viewDesc);

                images[i] = image;
                imageViews[i] = imageView;
            }
        }
        else
        {
            auto* image = device->CreateImage(imageDesc);
            auto* imageView = image->CreateView(viewDesc);

            images.push_back(image);
            imageViews.push_back(imageView);
        }

        return std::make_unique<GPURenderTextureAsset>(uuid,
                                                       sourceVersion,
                                                       renderer,
                                                       resolvedDesc,
                                                       std::move(images),
                                                       std::move(imageViews),
                                                       lastReferencedFrame);
    }

    GPURenderTextureAsset::GPURenderTextureAsset(UUID uuid,
                                                 uint64_t sourceVersion,
                                                 Renderer* renderer,
                                                 const RenderTextureDesc& desc,
                                                 std::vector<RHIImage*> images,
                                                 std::vector<RHIImageView*> imageViews,
                                                 uint64_t lastReferencedFrame)
        : GPUAsset(uuid, AssetType::RenderTexture, renderer, sourceVersion, lastReferencedFrame),
          m_IsValid(true),
          m_PerFrame(desc.perFrame),
          m_MaxFramesInFlight(renderer->GetMaxFramesInFlight()),
          m_Desc(desc),
          m_Images(std::move(images)),
          m_ImageViews(std::move(imageViews)) {}

    void GPURenderTextureAsset::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        for (auto& image : m_Images)
        {
            image->Release();
            image = nullptr;
        }

        m_IsValid = false;
    }

    void GPURenderTextureAsset::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        for (auto& image : m_Images)
        {
            image->ReleaseImmediate();
            image = nullptr;
        }

        m_IsValid = false;
    }

    void GPURenderTextureAsset::GenerateMipmap(RHICommandBuffer* commandBuffer)
    {
        auto* image = GetImage();
        ImageUtilGenerateMipmap(commandBuffer, image);
    }

    RHIImage* GPURenderTextureAsset::GetImage() const
    {
        return m_Images[m_PerFrame ? m_Renderer->GetCurrentFrameInFlightIndex() : 0];
    }

    const std::vector<RHIImage*>& GPURenderTextureAsset::GetAllImages() const
    {
        return m_Images;
    }

    RHIImageView* GPURenderTextureAsset::GetDefaultImageView() const
    {
        return m_ImageViews[m_PerFrame ? m_Renderer->GetCurrentFrameInFlightIndex() : 0];
    }
} // namespace Hazel