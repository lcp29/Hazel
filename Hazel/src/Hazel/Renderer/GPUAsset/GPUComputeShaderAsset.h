//
// Created by helmholtz on 2026/3/29.
//

#pragma once

#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"

#include <vector>

namespace Hazel
{
    class GPUComputeShaderAsset : public GPUAsset
    {
      public:
        GPUComputeShaderAsset() = delete;

        GPUComputeShaderAsset(UUID uuid,
                              uint64_t sourceVersion,
                              Renderer* renderer,
                              RHIShader* computeShader,
                              std::vector<RHIResourceLayout*> resourceLayouts,
                              RHIResourceSignature* resourceSignature,
                              RHIComputePipeline* cachedPipeline,
                              uint64_t lastReferencedFrame = 0)
            : GPUAsset(uuid, AssetType::ComputeShader, renderer, sourceVersion, lastReferencedFrame)
            , m_IsValid(true)
            , m_ComputeShader(computeShader)
            , m_ResourceLayouts(std::move(resourceLayouts))
            , m_ResourceSignature(resourceSignature)
            , m_CachedPipeline(cachedPipeline)
        {}

        ~GPUComputeShaderAsset() override;

        bool IsValid() const { return m_IsValid; }

        RHIShader* GetShader() const { return m_ComputeShader; }

        RHIResourceSignature* GetResourceSignature() const { return m_ResourceSignature; }

        const std::vector<RHIResourceLayout*>& GetResourceLayouts() const { return m_ResourceLayouts; }

        void Release() override;
        void ReleaseImmediate() override;

      private:
        bool m_IsValid = false;
        RHIShader* m_ComputeShader = nullptr;
        std::vector<RHIResourceLayout*> m_ResourceLayouts;
        RHIResourceSignature* m_ResourceSignature = nullptr;
        RHIComputePipeline* m_CachedPipeline = nullptr;
    };
} // namespace Hazel