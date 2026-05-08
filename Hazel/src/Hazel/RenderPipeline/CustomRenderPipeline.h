//
// Created by helmholtz on 2026/4/14.
//

#pragma once
#include "../Renderer/RenderPipeline.h"

namespace Hazel
{
    class CustomRenderPipeline : public RenderPipeline
    {
    public:
        CustomRenderPipeline(Renderer* renderer);

        void Render(RenderContext& context, const SceneCameraView& camera) override;

		~CustomRenderPipeline() override;

    private:
        GPUAssetHandle m_DepthStencilTexture = nullptr;
    };
} // namespace Hazel
