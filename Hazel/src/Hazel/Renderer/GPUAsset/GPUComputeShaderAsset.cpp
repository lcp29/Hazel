//
// Created by helmholtz on 2026/3/29.
//

#include "Hazel/Renderer/GPUAsset/GPUComputeShaderAsset.h"

namespace Hazel
{
    void GPUComputeShaderAsset::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        if (m_ResourceSignature)
        {
            m_ResourceSignature->Release();
            m_ResourceSignature = nullptr;
        }

        for (auto* resourceLayout : m_ResourceLayouts)
        {
            resourceLayout->Release();
        }
        m_ResourceLayouts.clear();

        m_ComputeShader->Release();
        m_ComputeShader = nullptr;

        m_IsValid = false;
    }

    void GPUComputeShaderAsset::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        if (m_ResourceSignature)
        {
            m_ResourceSignature->ReleaseImmediate();
            m_ResourceSignature = nullptr;
        }

        for (auto* resourceLayout : m_ResourceLayouts)
        {
            resourceLayout->ReleaseImmediate();
        }
        m_ResourceLayouts.clear();

        m_ComputeShader->ReleaseImmediate();
        m_ComputeShader = nullptr;

        m_IsValid = false;
    }
} // namespace Hazel