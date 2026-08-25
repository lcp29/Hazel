// Implements image loading and format utilities.
// Created: 2026-03-31.

#include "ImageUtils.h"

namespace Aster
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

    void ImageUtilGenerateMipmap(RHICommandBuffer* commandBuffer, RHIImage* image)
    {
        const RHIImageDesc& imageDesc = image->GetDesc();
        if (imageDesc.mipLevels <= 1) { return; }

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
                {{mipLevel - 1, 0, 1, RHIImagePlaneFlagBits::Color},
                 {{0, 0, 0}, {static_cast<int32_t>(srcWidth), static_cast<int32_t>(srcHeight), 1}},
                 {mipLevel, 0, 1, RHIImagePlaneFlagBits::Color},
                 {{0, 0, 0}, {static_cast<int32_t>(dstWidth), static_cast<int32_t>(dstHeight), 1}}});

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
} // namespace Aster
