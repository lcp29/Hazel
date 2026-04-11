//
// Created by helmholtz on 2026/3/28.
//

#include "GPUMeshAsset.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    GPUMeshAsset::~GPUMeshAsset()
    {
        GPUMeshAsset::ReleaseImmediate();
    }

    void GPUMeshAsset::Release()
    {
        // TODO: TEMP URGENT INTERVIEW: temporary vertex/index buffer path
        // m_Renderer->GetGeometryDataRegistry()->UnregisterMesh(this);
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
        // TODO: TEMP URGENT INTERVIEW: temporary vertex/index buffer path
        // m_Renderer->GetGeometryDataRegistry()->UnregisterMesh(this);
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
} // Hazel