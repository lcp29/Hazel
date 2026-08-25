// Implements GPU compute shader asset resources.
// Created: 2026-03-29.

#include "Hazel/Renderer/GPUAsset/GPUComputeShaderAsset.h"

namespace Aster
{
    GPUComputeShaderAsset::~GPUComputeShaderAsset() { GPUComputeShaderAsset::ReleaseImmediate(); }

    void GPUComputeShaderAsset::Release()
    {
        if (!m_IsValid) { return; }

        if (m_CachedPipeline)
        {
            m_CachedPipeline->Release();
            m_CachedPipeline = nullptr;
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
        if (!m_IsValid) { return; }

        if (m_CachedPipeline)
        {
            m_CachedPipeline->ReleaseImmediate();
            m_CachedPipeline = nullptr;
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
} // namespace Aster
