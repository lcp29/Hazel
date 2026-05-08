//
// Created by helmholtz on 2026/4/14.
//

#include "RenderContext.h"

#include "Renderer.h"

namespace Hazel
{
    void RenderContext::DrawAllObjects(const SceneCameraView& camera, const DrawSetting& drawSetting)
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

    void RenderContext::DrawAllObjectsMaterialOverride(const SceneCameraView& camera,
                                                       const DrawSetting& drawSetting,
                                                       UUID material)
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
    {
        GetRenderer()->GetResourceBindingRegistry()->SetBuffer(name, handle);
    }

    void RenderContext::SetImage(const std::string& name, const GPUAssetHandle* handle)
    {
        GetRenderer()->GetResourceBindingRegistry()->SetImage(name, handle);
    }

    void RenderContext::SetSampler(const std::string& name, const GPUAssetHandle* handle)
    {
        GetRenderer()->GetResourceBindingRegistry()->SetSampler(name, handle);
    }
} // namespace Hazel
