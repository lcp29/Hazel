// Declares GPU graphics pipeline asset resources.
// Created: 2026-04-05.

#pragma once
#include "GPUAsset.h"

#include <condition_variable>
#include <mutex>

namespace Hazel
{
    class Renderer;
}

namespace Aster
{

    class GPUGraphicsPipelineAsset : public GPUAsset
    {
      public:
        GPUGraphicsPipelineAsset() = delete;

        GPUGraphicsPipelineAsset(const Hazel::UUID& uuid,
                                 AssetType assetType,
                                 RHIGraphicsPipeline* pipeline,
                                 Hazel::UUID shader,
                                 Hazel::Renderer* renderer,
                                 uint64_t lastReferencedFrame)
            : GPUAsset(uuid, assetType, renderer, 0, lastReferencedFrame)
            , m_Isvalid(true)
            , m_Shader(shader)
            , m_Pipeline(pipeline)
        {}

        ~GPUGraphicsPipelineAsset() override;

        std::mutex& GetMutex() { return m_Mutex; }

        std::condition_variable& GetCondition() { return m_Condition; }

        bool IsLoading() const { return m_IsLoading; }

        RHIGraphicsPipeline* GetPipeline() const { return m_Pipeline; }

        void SetPipeline(RHIGraphicsPipeline* pipeline) { m_Pipeline = pipeline; }

        void SetLoading(bool isLoading) { m_IsLoading = isLoading; }

        void Release() override;
        void ReleaseImmediate() override;

      private:
        bool m_Isvalid = false;
        bool m_IsLoading = true;
        Hazel::UUID m_Shader = Hazel::UUID(-1);
        RHIGraphicsPipeline* m_Pipeline = nullptr;
        std::mutex m_Mutex;
        std::condition_variable m_Condition;
    };
} // namespace Aster
