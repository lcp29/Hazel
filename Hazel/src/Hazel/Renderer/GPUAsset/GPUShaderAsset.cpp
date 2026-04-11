//
// Created by helmholtz on 2026/3/31.
//

#include "Hazel/Renderer/GPUAsset/GPUShaderAsset.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    GPUShaderAsset::GPUShaderAsset(UUID uuid,
                                   uint64_t sourceVersion,
                                   Renderer* renderer,
                                   RHIShader* vertexShader,
                                   RHIShader* fragmentShader,
                                   RHIShaderReflection reflection,
                                   uint64_t lastReferencedFrame)
        : GPUAsset(uuid, AssetType::Shader, renderer, sourceVersion, lastReferencedFrame),
          m_IsValid(true),
          m_VertexShader(vertexShader),
          m_FragmentShader(fragmentShader),
          m_Reflection(reflection)
    {
    }

    GPUShaderAsset::~GPUShaderAsset()
    {
        GPUShaderAsset::ReleaseImmediate();
    }

    void GPUShaderAsset::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_Renderer->UnregisterShader(GetUUID(), GetSourceVersion());

        m_IsValid = false;

        m_VertexShader->Release();
        m_FragmentShader->Release();
        m_VertexShader = nullptr;
        m_FragmentShader = nullptr;
    }

    void GPUShaderAsset::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_Renderer->UnregisterShader(GetUUID(), GetSourceVersion());

        m_IsValid = false;

        m_VertexShader->ReleaseImmediate();
        m_FragmentShader->ReleaseImmediate();
        m_VertexShader = nullptr;
        m_FragmentShader = nullptr;
    }
} // namespace Hazel