// Declares render context.
// Created: 2026-04-14.

#pragma once
#include "../Core/UUID.h"
#include "../RHI/RHI.h"
#include "../Scene/Scene.h"
#include "GPUAsset/GPUAssetHandle.h"
#include "GPUAsset/GPURenderTextureAsset.h"

namespace Hazel
{
    class Renderer;
}

namespace Aster
{
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

        RenderContext(Hazel::Renderer* renderer, GPURenderTextureAsset* cameraRenderTexture, RHIExtent2D viewportSize)
            : m_Renderer(renderer)
            , m_CameraRenderTexture(cameraRenderTexture)
            , m_ViewportSize(viewportSize)
        {}

        Hazel::Renderer* GetRenderer() const { return m_Renderer; }

        GPURenderTextureAsset* GetCameraRenderTexture() const { return m_CameraRenderTexture; }

        RHIExtent2D GetViewportSize() const { return m_ViewportSize; }

        void DrawAllObjects(const Hazel::SceneCameraView& camera, const DrawSetting& drawSetting);

        void DrawAllObjectsMaterialOverride(const Hazel::SceneCameraView& camera,
                                            const DrawSetting& drawSetting,
                                            Hazel::UUID material);

        void SetBuffer(const std::string& name, const GPUAssetHandle* handle);
        void SetImage(const std::string& name, const GPUAssetHandle* handle);
        void SetSampler(const std::string& name, const GPUAssetHandle* handle);

      private:
        Hazel::Renderer* m_Renderer = nullptr;
        GPURenderTextureAsset* m_CameraRenderTexture = nullptr;
        RHIExtent2D m_ViewportSize;
    };
} // namespace Aster
