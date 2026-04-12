//
// Created by helmholtz on 2026/3/31.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"

#include <vector>

namespace Hazel
{
    class Renderer;

    class GPUShaderAsset : public GPUAsset
    {
      public:
        GPUShaderAsset() = delete;

        GPUShaderAsset(UUID uuid,
                       uint64_t sourceVersion,
                       Renderer* renderer,
                       RHIShader* vertexShader,
                       RHIShader* fragmentShader,
                       RHIShaderReflection reflection,
                       uint64_t lastReferencedFrame = 0);

        ~GPUShaderAsset() override;

        bool IsValid() const { return m_IsValid; }

        RHIShader* GetVertexShader() const { return m_VertexShader; }

        RHIShader* GetFragmentShader() const { return m_FragmentShader; }

        const RHIShaderReflection& GetReflection() const { return m_Reflection; }

        void Release() override;
        void ReleaseImmediate() override;

      private:
        bool m_IsValid = false;
        RHIShader* m_VertexShader = nullptr;
        RHIShader* m_FragmentShader = nullptr;
        RHIShaderReflection m_Reflection{};
    };
} // namespace Hazel