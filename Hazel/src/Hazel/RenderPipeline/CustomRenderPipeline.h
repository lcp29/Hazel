// Declares the custom render pipeline.
// Created: 2026-04-14.

#pragma once
#include "../Renderer/RenderPipeline.h"

namespace Aster
{
    class CustomRenderPipeline : public RenderPipeline
    {
      public:
        CustomRenderPipeline(Hazel::Renderer* renderer);

        void Render(RenderContext& context, const Hazel::SceneCameraView& camera) override;

        ~CustomRenderPipeline() override;

      private:
        GPUAssetHandle m_DepthStencilTexture = nullptr;
    };
} // namespace Aster
