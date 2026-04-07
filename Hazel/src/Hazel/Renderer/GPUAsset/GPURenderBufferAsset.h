//
// Created by helmholtz on 2026/4/1.
//

#pragma once
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"
#include "Hazel/RHI/RHI.h"

#include <memory>
#include <vector>

namespace Hazel
{
    class Renderer;

    struct RenderBufferDesc
    {
        bool perFrame = true;
        uint64_t size = 0;
        RHIBufferUsages usages = {};
        RHIBufferCpuAccess cpuAccess = RHIBufferCpuAccess::None;
        bool mapOnCreate = false;
        bool allowGpuAddress = false;
        bool hostCoherent = false;
    };

    class GPURenderBufferAsset : public GPUAsset
    {
    public:
        GPURenderBufferAsset() = delete;

        GPURenderBufferAsset(UUID uuid,
                             uint64_t sourceVersion,
                             Renderer* renderer,
                             const RenderBufferDesc& desc,
                             std::vector<RHIBuffer*> buffers,
                             uint64_t lastReferencedFrame = 0);

        void Release() override;
        void ReleaseImmediate() override;

        RHIBuffer* GetBuffer() const;

        const std::vector<RHIBuffer*>& GetAllBuffers() const
        {
            return m_Buffers;
        }

        const RenderBufferDesc& GetDesc() const
        {
            return m_Desc;
        }

        bool IsValid() const
        {
            return m_IsValid;
        }

    private:
        bool m_IsValid = false;
        bool m_PerFrame = true;
        RenderBufferDesc m_Desc{};
        uint32_t m_MaxFramesInFlight = 0;
        std::vector<RHIBuffer*> m_Buffers;
    };

    std::unique_ptr<GPURenderBufferAsset> CreateGPURenderBufferAsset(Renderer* renderer,
                                                                     UUID uuid,
                                                                     uint64_t sourceVersion,
                                                                     const RenderBufferDesc& desc,
                                                                     uint64_t lastReferencedFrame = 0);
} // Hazel