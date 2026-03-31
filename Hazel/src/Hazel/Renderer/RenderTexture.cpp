//
// Created by helmholtz on 2026/3/22.
//

#include "Hazel/Renderer/RenderTexture.h"
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

    RenderTexture::RenderTexture(UUID uuid, Renderer* renderer, const RenderTextureDesc& desc)
        : m_PerFrame(desc.perFrame),
          m_UUID(uuid),
          m_MaxFramesInFlight(renderer->GetMaxFramesInFlight()),
          m_Desc(desc),
          m_Renderer(renderer)
    {
        auto* device = renderer->GetDevice();

        RHIImageDesc imageDesc{};
        imageDesc.width = m_Desc.width;
        imageDesc.height = m_Desc.height;
        imageDesc.depth = m_Desc.depth;
        imageDesc.mipLevels = m_Desc.useMipmap ? DeduceMipLevelCount(m_Desc.width, m_Desc.height) : 1;
        imageDesc.arrayLayers = m_Desc.arrayLayers;
        imageDesc.format = m_Desc.format;
        imageDesc.usages = RHIImageUsageFlagBits::Sampled | desc.usages;

        auto imagePlanes = GetImagePlanesFromFormat(desc.format);

        if (imagePlanes & RHIImagePlaneFlagBits::Depth || imagePlanes & RHIImagePlaneFlagBits::Stencil)
        {
            imageDesc.usages |= RHIImageUsageFlagBits::DepthStencilAttachment;
        }
        else
        {
            imageDesc.usages |= RHIImageUsageFlagBits::ColorAttachment;
        }

        if (m_Desc.useMipmap)
        {
            imageDesc.usages |= RHIImageUsageFlagBits::TransferSource | RHIImageUsageFlagBits::TransferDestination;
        }

        m_Desc.usages = imageDesc.usages;

        RHIImageViewDesc viewDesc{};
        viewDesc.viewType = m_Desc.viewType;
        viewDesc.format = imageDesc.format;
        viewDesc.subresourceRange.baseMipLevel = 0;
        viewDesc.subresourceRange.levelCount = imageDesc.mipLevels;
        viewDesc.subresourceRange.baseArrayLayer = 0;
        viewDesc.subresourceRange.layerCount = m_Desc.arrayLayers;
        viewDesc.subresourceRange.planes = imagePlanes;
        imageDesc.initialState = RHIImageResourceState::Undefined;

        if (m_Desc.perFrame)
        {
            m_Images.resize(m_MaxFramesInFlight, nullptr);
            m_ImageViews.resize(m_MaxFramesInFlight, nullptr);

            for (int i = 0; i < renderer->GetMaxFramesInFlight(); i++)
            {
                RHIImage* image = device->CreateImage(imageDesc);
                RHIImageView* imageView = image->CreateView(viewDesc);

                m_Images[i] = image;
                m_ImageViews[i] = imageView;
            }
        }
        else
        {
            RHIImage* image = device->CreateImage(imageDesc);
            RHIImageView* imageView = image->CreateView(viewDesc);

            m_Images.push_back(image);
            m_ImageViews.push_back(imageView);
        }

        m_IsValid = true;
    }

    void RenderTexture::Release()
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

    void RenderTexture::ReleaseImmediate()
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

    void RenderTexture::GenerateMipmap(RHICommandBuffer* commandBuffer)
    {
        auto* image = GetImage();
        ImageUtilGenerateMipmap(commandBuffer, image);
    }

    RHIImage* RenderTexture::GetImage() const
    {
        return m_Images[m_PerFrame ? m_Renderer->GetCurrentFrameInFlightIndex() : 0];
    }

    const std::vector<RHIImage*>& RenderTexture::GetAllImages() const
    {
        return m_Images;
    }

    RHIImageView* RenderTexture::GetImageView() const
    {
        return m_ImageViews[m_PerFrame ? m_Renderer->GetCurrentFrameInFlightIndex() : 0];
    }
} // namespace Hazel