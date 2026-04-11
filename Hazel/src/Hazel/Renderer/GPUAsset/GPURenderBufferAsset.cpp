//
// Created by helmholtz on 2026/4/1.
//

#include "Hazel/Renderer/GPUAsset/GPURenderBufferAsset.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    std::unique_ptr<GPURenderBufferAsset> CreateGPURenderBufferAsset(Renderer* renderer,
                                                                     UUID uuid,
                                                                     uint64_t sourceVersion,
                                                                     const RenderBufferDesc& desc,
                                                                     uint64_t lastReferencedFrame)
    {
        RHIBufferDesc bufferDesc{};
        bufferDesc.usages = desc.usages;
        bufferDesc.size = desc.size;
        bufferDesc.hostCoherent = desc.hostCoherent;
        bufferDesc.allowGpuAddress = desc.allowGpuAddress;
        bufferDesc.cpuAccess = desc.cpuAccess;
        bufferDesc.mapOnCreate = desc.mapOnCreate;
        bufferDesc.deviceMemory = desc.deviceMemory;

        std::vector<RHIBuffer*> buffers;

        if (desc.perFrame)
        {
            buffers.resize(renderer->GetMaxFramesInFlight(), nullptr);
            for (int i = 0; i < renderer->GetMaxFramesInFlight(); i++)
            {
                buffers[i] = renderer->GetDevice()->CreateBuffer(bufferDesc);
            }
        }
        else
        {
            buffers.push_back(renderer->GetDevice()->CreateBuffer(bufferDesc));
        }

        return std::make_unique<GPURenderBufferAsset>(uuid,
                                                      sourceVersion,
                                                      renderer,
                                                      desc,
                                                      std::move(buffers),
                                                      lastReferencedFrame);
    }

    GPURenderBufferAsset::GPURenderBufferAsset(UUID uuid,
                                               uint64_t sourceVersion,
                                               Renderer* renderer,
                                               const RenderBufferDesc& desc,
                                               std::vector<RHIBuffer*> buffers,
                                               uint64_t lastReferencedFrame)
        : GPUAsset(uuid, AssetType::RenderBuffer, renderer, sourceVersion, lastReferencedFrame),
          m_IsValid(true),
          m_PerFrame(desc.perFrame),
          m_Desc(desc),
          m_MaxFramesInFlight(renderer->GetMaxFramesInFlight()),
          m_Buffers(std::move(buffers)) {}

    GPURenderBufferAsset::~GPURenderBufferAsset()
    {
        GPURenderBufferAsset::ReleaseImmediate();
    }

    void GPURenderBufferAsset::Release()
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

    void GPURenderBufferAsset::ReleaseImmediate()
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

    RHIBuffer* GPURenderBufferAsset::GetBuffer() const
    {
        return m_PerFrame ? m_Buffers[m_Renderer->GetCurrentFrameInFlightIndex()] : m_Buffers[0];
    }
} // Hazel