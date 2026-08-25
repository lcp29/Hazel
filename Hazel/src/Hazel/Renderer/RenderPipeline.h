// Declares render pipeline.
// Created: 2026-04-14.

#pragma once
#include "../Scene/Scene.h"
#include "RenderContext.h"

namespace Aster
{
    class RenderPipeline
    {
      public:
        RenderPipeline() = default;
        virtual void Render(RenderContext& context, const Hazel::SceneCameraView& camera) = 0;
        virtual ~RenderPipeline() = default;
    };
} // namespace Aster
