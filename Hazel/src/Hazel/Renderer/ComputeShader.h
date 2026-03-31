//
// Created by helmholtz on 2026/3/29.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class ComputeShader
    {
    public:
        ComputeShader() = delete;

        ComputeShader(UUID uuid, RHIShader* shader)
            : m_IsValid(true), m_UUID(uuid), m_ComputeShader(shader) {};
        ComputeShader(const ComputeShader&) = delete;
        ComputeShader& operator=(const ComputeShader&) = delete;
        ComputeShader(ComputeShader&& other) noexcept;
        ComputeShader& operator=(ComputeShader&& other) noexcept;
        ~ComputeShader();

        bool IsValid() const
        {
            return m_IsValid;
        }

        RHIShader* GetShader() const
        {
            return m_ComputeShader;
        }

        void Release();
        void ReleaseImmediate();

    private:
        bool m_IsValid = false;
        UUID m_UUID = 0;
        RHIShader* m_ComputeShader = nullptr;
    };
} // namespace Hazel