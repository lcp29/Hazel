//
// Created by helmholtz on 2026/4/14.
//

#pragma once
#include "RenderContext.h"
#include "../Scene/Scene.h"

namespace Hazel
{
    class RenderPipeline
    {
      public:
        RenderPipeline() = default;
        virtual void Render(RenderContext& context, const SceneCameraView& camera) = 0;
        virtual ~RenderPipeline() = default;
    };
} // namespace Hazel
