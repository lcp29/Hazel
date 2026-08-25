// Declares GPU shader asset resources.
// Created: 2026-03-31.

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"

#include <vector>

namespace Hazel
{
    class Renderer;
}

namespace Aster
{

    class GPUShaderAsset : public GPUAsset
    {
      public:
        GPUShaderAsset() = delete;

        GPUShaderAsset(Hazel::UUID uuid,
                       uint64_t sourceVersion,
                       Hazel::Renderer* renderer,
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
} // namespace Aster
