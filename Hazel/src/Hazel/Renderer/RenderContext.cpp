// Implements render context.
// Created: 2026-04-14.

#include "RenderContext.h"

#include "Renderer.h"

namespace Aster
{
    void RenderContext::DrawAllObjects(const Hazel::SceneCameraView& camera, const DrawSetting& drawSetting)
    {
        m_Renderer->RunGraphicsPass(m_Renderer->GetCurrentFrameData().commandBuffer,
                                    camera,
                                    drawSetting.colorAttachments,
                                    drawSetting.colorBlendDesc,
                                    drawSetting.depthStencilAttachment,
                                    drawSetting.renderArea,
                                    drawSetting.viewportArea,
                                    drawSetting.scissorArea);
    }

    void RenderContext::DrawAllObjectsMaterialOverride(const Hazel::SceneCameraView& camera,
                                                       const DrawSetting& drawSetting,
                                                       Hazel::UUID material)
    {
        m_Renderer->RunGraphicsPass(m_Renderer->GetCurrentFrameData().commandBuffer,
                                    material,
                                    camera,
                                    drawSetting.colorAttachments,
                                    drawSetting.colorBlendDesc,
                                    drawSetting.depthStencilAttachment,
                                    drawSetting.renderArea,
                                    drawSetting.viewportArea,
                                    drawSetting.scissorArea);
    }

    void RenderContext::SetBuffer(const std::string& name, const GPUAssetHandle* handle)
    { GetRenderer()->GetResourceBindingRegistry()->SetBuffer(name, handle); }

    void RenderContext::SetImage(const std::string& name, const GPUAssetHandle* handle)
    { GetRenderer()->GetResourceBindingRegistry()->SetImage(name, handle); }

    void RenderContext::SetSampler(const std::string& name, const GPUAssetHandle* handle)
    { GetRenderer()->GetResourceBindingRegistry()->SetSampler(name, handle); }
} // namespace Aster
