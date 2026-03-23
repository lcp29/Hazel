//
// Created by helmholtz on 2026/3/22.
//

#include "Hazel/Renderer/RenderTexture.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    namespace
    {
        uint32_t DeduceMipLevelCount(uint32_t width, uint32_t height)
        {
            uint32_t maxDimension = width > height ? width : height;
            uint32_t mipLevels = 1;
            while (maxDimension > 1)
            {
                maxDimension >>= 1;
                ++mipLevels;
            }

            return mipLevels;
        }
    } // namespace

    RenderTexture::RenderTexture(Renderer* renderer, const RenderTextureDesc& desc)
        : m_MaxFramesInFlight(renderer->GetMaxFramesInFlight())
          , m_Desc(desc)
          , m_Renderer(renderer)
    {
        auto* device = renderer->GetDevice();

        RHIImageDesc colorImageDesc{};
        colorImageDesc.width = m_Desc.width;
        colorImageDesc.height = m_Desc.height;
        colorImageDesc.depth = 1;
        colorImageDesc.mipLevels = m_Desc.useMipmap ? DeduceMipLevelCount(m_Desc.width, m_Desc.height) : 1;
        colorImageDesc.arrayLayers = 1;
        colorImageDesc.format = m_Desc.format;
        colorImageDesc.usages = RHIImageUsageFlagBits::ColorAttachment | RHIImageUsageFlagBits::Sampled | desc.usages;

        if (m_Desc.useMipmap)
        {
            colorImageDesc.usages |= RHIImageUsageFlagBits::TransferSource | RHIImageUsageFlagBits::TransferDestination;
        }

        RHIImageViewDesc colorViewDesc{};
        colorViewDesc.viewType = RHIImageViewType::Image2D;
        colorViewDesc.format = colorImageDesc.format;
        colorViewDesc.subresourceRange.baseMipLevel = 0;
        colorViewDesc.subresourceRange.levelCount = 1;
        colorViewDesc.subresourceRange.baseArrayLayer = 0;
        colorViewDesc.subresourceRange.layerCount = 1;
        colorViewDesc.subresourceRange.planes = RHIImagePlaneFlagBits::Color;
        colorImageDesc.initialState = RHIImageResourceState::Undefined;

        if (m_Desc.perFrame)
        {
            m_Images.resize(m_MaxFramesInFlight, nullptr);
            m_ImageViews.resize(m_MaxFramesInFlight, nullptr);

            for (int i = 0; i < renderer->GetMaxFramesInFlight(); i++)
            {
                RHIImageView* imageView = nullptr;
                RHIImage* image = device->CreateImage(colorImageDesc);

                if (image)
                {
                    imageView = image->CreateView(colorViewDesc);
                }

                m_Images[i] = image;
                m_ImageViews[i] = imageView;

                if (!(image && imageView))
                {
                    ReleaseImmediate();
                    return;
                }
            }
        }
        else
        {
            RHIImageView* imageView = nullptr;
            RHIImage* image = device->CreateImage(colorImageDesc);
            if (image)
            {
                imageView = image->CreateView(colorViewDesc);
            }

            m_Images.push_back(image);
            m_ImageViews.push_back(imageView);

            if (!(image && imageView))
            {
                ReleaseImmediate();
                return;
            }
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
            if (image)
            {
                image->Release();
                image = nullptr;
            }
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
            if (image)
            {
                image->ReleaseImmediate();
                image = nullptr;
            }
        }

        m_IsValid = false;
    }

    void RenderTexture::GenerateMipmap(RHICommandBuffer* commandBuffer)
    {
        HZ_RHI_DEBUG_RETURN_IF(!commandBuffer);

        for (auto& image : m_Images)
        {
            const RHIImageDesc& imageDesc = image->GetDesc();
            if (imageDesc.mipLevels <= 1)
                return;

            const RHIImageResourceState originalState = image->GetCurrentState();

            RHIImageSubresourceRange destinationRange{};
            destinationRange.baseMipLevel = 1;
            destinationRange.levelCount = imageDesc.mipLevels - 1;
            destinationRange.baseArrayLayer = 0;
            destinationRange.layerCount = 1;
            destinationRange.planes = RHIImagePlaneFlagBits::Color;
            image->Transition(commandBuffer,
                              RHIImageResourceState::Undefined,
                              RHIImageResourceState::TransferDestination,
                              destinationRange);

            RHIImageSubresourceRange sourceRange{};
            sourceRange.baseMipLevel = 0;
            sourceRange.levelCount = 1;
            sourceRange.baseArrayLayer = 0;
            sourceRange.layerCount = 1;
            sourceRange.planes = RHIImagePlaneFlagBits::Color;
            image->Transition(commandBuffer, originalState, RHIImageResourceState::TransferSource, sourceRange);

            uint32_t srcWidth = imageDesc.width;
            uint32_t srcHeight = imageDesc.height;
            for (uint32_t mipLevel = 1; mipLevel < imageDesc.mipLevels; ++mipLevel)
            {
                const uint32_t dstWidth = srcWidth > 1 ? srcWidth / 2 : 1;
                const uint32_t dstHeight = srcHeight > 1 ? srcHeight / 2 : 1;

                RHIImageBlitDesc blitDesc{};
                blitDesc.filter = RHIBlitFilter::Linear;
                blitDesc.regions.push_back(
                    {
                        // source subresource
                        {mipLevel - 1, 0, 1, RHIImagePlaneFlagBits::Color},
                        {{0, 0, 0}, {static_cast<int32_t>(srcWidth), static_cast<int32_t>(srcHeight), 1}},
                        {mipLevel, 0, 1, RHIImagePlaneFlagBits::Color},
                        {{0, 0, 0}, {static_cast<int32_t>(dstWidth), static_cast<int32_t>(dstHeight), 1}}
                    });

                commandBuffer->BlitImage(image,
                                         RHIImageResourceState::TransferSource,
                                         image,
                                         RHIImageResourceState::TransferDestination,
                                         blitDesc);

                if (mipLevel < imageDesc.mipLevels - 1)
                {
                    RHIImageSubresourceRange promotedRange{};
                    promotedRange.baseMipLevel = mipLevel;
                    promotedRange.levelCount = 1;
                    promotedRange.baseArrayLayer = 0;
                    promotedRange.layerCount = 1;
                    promotedRange.planes = RHIImagePlaneFlagBits::Color;
                    image->Transition(commandBuffer,
                                      RHIImageResourceState::TransferDestination,
                                      RHIImageResourceState::TransferSource,
                                      promotedRange);
                }

                srcWidth = dstWidth;
                srcHeight = dstHeight;
            }

            for (uint32_t mipLevel = 0; mipLevel < imageDesc.mipLevels; ++mipLevel)
            {
                RHIImageSubresourceRange shaderReadRange{};
                shaderReadRange.baseMipLevel = mipLevel;
                shaderReadRange.levelCount = 1;
                shaderReadRange.baseArrayLayer = 0;
                shaderReadRange.layerCount = 1;
                shaderReadRange.planes = RHIImagePlaneFlagBits::Color;

                const RHIImageResourceState oldState = mipLevel == imageDesc.mipLevels - 1
                                                           ? RHIImageResourceState::TransferDestination
                                                           : RHIImageResourceState::TransferSource;
                image->Transition(commandBuffer, oldState, RHIImageResourceState::ShaderRead, shaderReadRange);
            }
        }
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