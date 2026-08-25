#include "Hazel/Asset/TextureAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/Importer/GPUAssetImporter.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Utils/ImageUtils.h"

#include <cstring>

namespace Aster
{
    std::unique_ptr<GPUTextureAsset> ImportGPUTextureAsset(Hazel::Renderer* renderer, const TextureAsset* asset)
    {
        const auto& meta = asset->GetMeta();
        const auto& textureData = asset->GetTextureData();

        TextureDesc textureDesc{};
        textureDesc.width = textureData.width;
        textureDesc.height = textureData.height;
        textureDesc.useMipmap = meta.UseMipmap();
        textureDesc.format = textureData.format;
        textureDesc.usages = RHIImageUsageFlagBits::Sampled;
        if (meta.AllowStorageLoad()) { textureDesc.usages |= RHIImageUsageFlagBits::Storage; }
        if (textureDesc.useMipmap)
        {
            textureDesc.usages |= RHIImageUsageFlagBits::TransferSource | RHIImageUsageFlagBits::TransferDestination;
        }

        RHIImageDesc imageDesc{};
        imageDesc.width = textureDesc.width;
        imageDesc.height = textureDesc.height;
        imageDesc.depth = 1;
        imageDesc.arrayLayers = 1;
        imageDesc.mipLevels = textureDesc.useMipmap ? DeduceMipLevelCount(textureDesc.width, textureDesc.height) : 1;
        imageDesc.format = textureDesc.format;
        imageDesc.usages = textureDesc.usages;
        imageDesc.initialState = RHIImageResourceState::Undefined;

        auto* graphicsContext = renderer->GetGraphicsContext();
        auto* device = renderer->GetDevice();
        auto* queue = device->GetUniformQueue();
        auto* cmd = graphicsContext->AcquireDefaultCommandBuffer();
        cmd->Begin(true);

        auto* image = device->CreateImage(imageDesc, false);

        image->Transition(cmd,
                          RHIImageResourceState::Undefined,
                          textureDesc.useMipmap ? RHIImageResourceState::TransferDestination
                                                : RHIImageResourceState::ShaderRead);

        RHIBufferDesc stagingBufferDesc{};
        stagingBufferDesc.size = textureData.rawImageData.size();
        stagingBufferDesc.usages = RHIBufferUsageFlagBits::TransferSource;
        stagingBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
        stagingBufferDesc.mapOnCreate = true;

        auto* stagingBuffer = device->CreateBuffer(stagingBufferDesc, true);
        void* mappedData = stagingBuffer->Map();

        std::memcpy(mappedData, textureData.rawImageData.data(), textureData.rawImageData.size());
        stagingBuffer->Unmap();

        image->Transition(cmd, RHIImageResourceState::Undefined, RHIImageResourceState::TransferDestination);

        cmd->CopyBufferToImage(stagingBuffer,
                               0,
                               {imageDesc.width, imageDesc.height},
                               image,
                               {0, 0, 0},
                               {imageDesc.width, imageDesc.height, 1},
                               {0, 0, 1, RHIImagePlaneFlagBits::Color});

        if (textureDesc.useMipmap) { ImageUtilGenerateMipmap(cmd, image); }
        else
        {
            image->Transition(cmd, RHIImageResourceState::TransferDestination, RHIImageResourceState::ShaderRead);
        }

        cmd->End();

        RHIQueueSubmitDesc submitDesc{};
        submitDesc.commandBuffers = {cmd};
        RHISyncPoint syncPoint = queue->Submit(submitDesc);
        stagingBuffer->Release();
        device->WaitSyncPoint(&syncPoint);
        graphicsContext->ReleaseDefaultCommandBuffer(cmd);

        RHIImageViewDesc imageViewDesc{};
        imageViewDesc.format = textureDesc.format;
        imageViewDesc.subresourceRange.levelCount = imageDesc.mipLevels;
        auto* imageView = device->CreateImageView(image, imageViewDesc);

        return std::make_unique<GPUTextureAsset>(asset->GetUUID(),
                                                 asset->GetVersion(),
                                                 textureDesc,
                                                 renderer,
                                                 image,
                                                 imageView,
                                                 renderer->GetCurrentFrameIndex());
    }
} // namespace Aster
