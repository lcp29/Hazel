//
// Created by helmholtz on 2026/4/1.
//

#include "RenderBuffer.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    RenderBuffer::RenderBuffer(Renderer* renderer, const RenderBufferDesc& desc)
        : m_PerFrame(desc.perFrame), m_Desc(desc), m_MaxFramesInFlight(renderer->GetMaxFramesInFlight()),
          m_Renderer(renderer)
    {
        RHIBufferDesc bufferDesc{};
        bufferDesc.usages = desc.usages;
        bufferDesc.size = desc.size;
        bufferDesc.hostCoherent = desc.hostCoherent;
        bufferDesc.allowGpuAddress = desc.allowGpuAddress;
        bufferDesc.cpuAccess = desc.cpuAccess;
        bufferDesc.mapOnCreate = desc.mapOnCreate;

        if (m_PerFrame)
        {
            m_Buffers.resize(m_MaxFramesInFlight, nullptr);
            for (int i = 0; i < m_MaxFramesInFlight; i++)
            {
                m_Buffers[i] = m_Renderer->GetDevice()->CreateBuffer(bufferDesc);
            }
        }
        else
        {
            m_Buffers.push_back(m_Renderer->GetDevice()->CreateBuffer(bufferDesc));
        }

        m_IsValid = true;
    }

    RenderBuffer::~RenderBuffer()
    {
        Release();
    }

    void RenderBuffer::Release()
    {
        if (!m_IsValid)
        {
            return;
        }
        for (auto* buffer : m_Buffers)
        {
            if (buffer)
            {
                buffer->Release();
            }
        }
        m_Buffers.clear();
        m_IsValid = false;
    }

    void RenderBuffer::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }
        for (auto* buffer : m_Buffers)
        {
            if (buffer)
            {
                buffer->ReleaseImmediate();
            }
        }
        m_Buffers.clear();
        m_IsValid = false;
    }

    RHIBuffer* RenderBuffer::GetBuffer() const
    {
        return m_PerFrame ? m_Buffers[m_Renderer->GetCurrentFrameInFlightIndex()] : m_Buffers[0];
    }
} // Hazel