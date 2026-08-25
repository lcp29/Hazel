// Implements GPU mesh asset resources.
// Created: 2026-03-28.

#include "GPUMeshAsset.h"

#include "Hazel/Renderer/Renderer.h"

namespace Aster
{
    GPUMeshAsset::~GPUMeshAsset() { GPUMeshAsset::ReleaseImmediate(); }

    void GPUMeshAsset::Release()
    {
        if (m_VertexBuffer)
        {
            m_VertexBuffer->Release();
            m_VertexBuffer = nullptr;
        }
        if (m_IndexBuffer)
        {
            m_IndexBuffer->Release();
            m_IndexBuffer = nullptr;
        }
    }

    void GPUMeshAsset::ReleaseImmediate()
    {
        if (m_VertexBuffer)
        {
            m_VertexBuffer->ReleaseImmediate();
            m_VertexBuffer = nullptr;
        }
        if (m_IndexBuffer)
        {
            m_IndexBuffer->ReleaseImmediate();
            m_IndexBuffer = nullptr;
        }
    }
} // namespace Aster
