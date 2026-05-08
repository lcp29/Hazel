//
// Created by helmholtz on 2026/4/14.
//

#include "CustomRenderPipeline.h"

#include "../Asset/Asset.h"
#include "../RHI/RHI.h"
#include "../Renderer/GPUAsset/GPURenderTextureAsset.h"
#include "../Renderer/Renderer.h"

namespace Hazel
{
    CustomRenderPipeline::CustomRenderPipeline(Renderer* renderer)
    {
        RenderTextureDesc depthStencilDesc{};
        depthStencilDesc.height = renderer->GetDefaultRenderTexture()->GetDesc().height;
        depthStencilDesc.width = renderer->GetDefaultRenderTexture()->GetDesc().width;
        depthStencilDesc.depth = 1;
        depthStencilDesc.format = RHIFormat::D32SFloatS8Uint;
        depthStencilDesc.perFrame = true;
        depthStencilDesc.arrayLayers = 1;
        depthStencilDesc.usages = RHIImageUsageFlagBits::DepthStencilAttachment;
        depthStencilDesc.useMipmap = false;
        m_DepthStencilTexture = renderer->ResolveGPURenderTexture(depthStencilDesc);
    }

    void CustomRenderPipeline::Render(RenderContext& context, const SceneCameraView& camera)
    {
        auto* renderTexture = context.GetCameraRenderTexture();

        renderTexture->GetImage()->Transition(context.GetRenderer()->GetCurrentFrameData().commandBuffer,
                                              renderTexture->GetImage()->GetCurrentState(),
                                              RHIImageResourceState::ColorAttachment);

        auto* depthStencilTexture = static_cast<GPURenderTextureAsset*>(m_DepthStencilTexture.asset);
        depthStencilTexture->GetImage()->Transition(context.GetRenderer()->GetCurrentFrameData().commandBuffer,
                                                    depthStencilTexture->GetImage()->GetCurrentState(),
                                                    RHIImageResourceState::DepthStencilAttachment);

        DrawSetting drawSetting{};

        RHIRenderingAttachmentDesc colorAttachmentDesc{};
        colorAttachmentDesc.imageView = context.GetCameraRenderTexture()->GetDefaultImageView();
        colorAttachmentDesc.state = RHIImageResourceState::ColorAttachment;
        colorAttachmentDesc.loadOp = RHIRenderingLoadOp::Clear;
        colorAttachmentDesc.storeOp = RHIRenderingStoreOp::Store;
        colorAttachmentDesc.clearColorValue.float32 = {0.0f, 0.0f, 0.0f, 1.0f};

        drawSetting.colorAttachments.push_back(colorAttachmentDesc);

        drawSetting.colorBlendDesc = {{}};

        RHIRenderingAttachmentDesc depthStencilAttachmentDesc{};
        depthStencilAttachmentDesc.imageView =
            static_cast<GPURenderTextureAsset*>(m_DepthStencilTexture.asset)->GetDefaultImageView();
        depthStencilAttachmentDesc.state = RHIImageResourceState::DepthStencilAttachment;
        depthStencilAttachmentDesc.loadOp = RHIRenderingLoadOp::Clear;
        depthStencilAttachmentDesc.storeOp = RHIRenderingStoreOp::Store;
        depthStencilAttachmentDesc.clearDepthStencilValue = {1.0f, 0};

        drawSetting.depthStencilAttachment = depthStencilAttachmentDesc;

        drawSetting.renderArea = {.offset = {0, 0},
                                  .extent = {context.GetViewportSize().width, context.GetViewportSize().height}};
        drawSetting.viewportArea = drawSetting.renderArea;
        drawSetting.scissorArea = drawSetting.renderArea;

        context.DrawAllObjects(camera, drawSetting);
    }

    CustomRenderPipeline::~CustomRenderPipeline() { m_DepthStencilTexture.Destroy(); }
} // namespace Hazel
