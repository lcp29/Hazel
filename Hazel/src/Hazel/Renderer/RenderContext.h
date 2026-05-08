//
// Created by helmholtz on 2026/4/14.
//

#pragma once
#include "../Core/UUID.h"
#include "../RHI/RHI.h"
#include "../Scene/Scene.h"
#include "GPUAsset/GPUAssetHandle.h"
#include "GPUAsset/GPURenderTextureAsset.h"

namespace Hazel
{
    class Renderer;

    struct DrawSetting
    {
        std::vector<RHIRenderingAttachmentDesc> colorAttachments;
        std::vector<RHIColorBlendAttachmentDesc> colorBlendDesc;
        RHIRenderingAttachmentDesc depthStencilAttachment;
        RHIRect2D renderArea;
        RHIRect2D viewportArea;
        RHIRect2D scissorArea;
    };

    class RenderContext
    {
      public:
        RenderContext() = delete;

        RenderContext(Renderer* renderer, GPURenderTextureAsset* cameraRenderTexture, RHIExtent2D viewportSize)
            : m_Renderer(renderer)
            , m_CameraRenderTexture(cameraRenderTexture)
            , m_ViewportSize(viewportSize)
        {}

        Renderer* GetRenderer() const { return m_Renderer; }

        GPURenderTextureAsset* GetCameraRenderTexture() const { return m_CameraRenderTexture; }

        RHIExtent2D GetViewportSize() const { return m_ViewportSize; }

        void DrawAllObjects(const SceneCameraView& camera, const DrawSetting& drawSetting);

        void
        DrawAllObjectsMaterialOverride(const SceneCameraView& camera, const DrawSetting& drawSetting, UUID material);

        void SetBuffer(const std::string& name, const GPUAssetHandle* handle);
        void SetImage(const std::string& name, const GPUAssetHandle* handle);
        void SetSampler(const std::string& name, const GPUAssetHandle* handle);

      private:
        Renderer* m_Renderer = nullptr;
        GPURenderTextureAsset* m_CameraRenderTexture = nullptr;
        RHIExtent2D m_ViewportSize;
    };
} // namespace Hazel
