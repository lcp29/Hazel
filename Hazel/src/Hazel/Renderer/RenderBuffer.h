//
// Created by helmholtz on 2026/4/1.
//

#pragma once
#include "Hazel/RHI/RHI.h"
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

    class RenderBuffer
    {
    public:
        RenderBuffer() = delete;
        RenderBuffer(Renderer* renderer, const RenderBufferDesc& desc);
        ~RenderBuffer();

        void Release();
        void ReleaseImmediate();

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
        Renderer* m_Renderer = nullptr;
        std::vector<RHIBuffer*> m_Buffers;
    };
} // Hazel