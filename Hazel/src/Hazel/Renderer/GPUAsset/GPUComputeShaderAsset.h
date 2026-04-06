//
// Created by helmholtz on 2026/3/29.
//

#pragma once

#include "Hazel/Renderer/GPUAsset/GPUAsset.h"
#include "Hazel/RHI/RHI.h"

#include <vector>

namespace Hazel
{
    class GPUComputeShaderAsset : public GPUAsset
    {
    public:
        GPUComputeShaderAsset() = delete;

        GPUComputeShaderAsset(UUID uuid,
                              uint64_t sourceVersion,
                              RHIShader* computeShader,
                              std::vector<RHIResourceLayout*> resourceLayouts,
                              RHIResourceSignature* resourceSignature,
                              uint64_t lastReferencedFrame = 0)
            : GPUAsset(uuid, AssetType::ComputeShader, sourceVersion, lastReferencedFrame),
              m_IsValid(true),
              m_ComputeShader(computeShader),
              m_ResourceLayouts(std::move(resourceLayouts)),
              m_ResourceSignature(resourceSignature) {}

        bool IsValid() const
        {
            return m_IsValid;
        }

        RHIShader* GetShader() const
        {
            return m_ComputeShader;
        }

        RHIResourceSignature* GetResourceSignature() const
        {
            return m_ResourceSignature;
        }

        const std::vector<RHIResourceLayout*>& GetResourceLayouts() const
        {
            return m_ResourceLayouts;
        }

        void Release();
        void ReleaseImmediate();

    private:
        bool m_IsValid = false;
        RHIShader* m_ComputeShader = nullptr;
        std::vector<RHIResourceLayout*> m_ResourceLayouts;
        RHIResourceSignature* m_ResourceSignature = nullptr;
    };
} // namespace Hazel