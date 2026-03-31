//
// Created by helmholtz on 2026/3/29.
//

#include "ComputeShader.h"

namespace Hazel
{
    ComputeShader::ComputeShader(ComputeShader&& other) noexcept
        : m_IsValid(other.m_IsValid),
          m_UUID(other.m_UUID),
          m_ComputeShader(other.m_ComputeShader)
    {
        other.m_IsValid = false;
        other.m_ComputeShader = nullptr;
    }

    ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_IsValid = other.m_IsValid;
        m_UUID = other.m_UUID;
        m_ComputeShader = other.m_ComputeShader;

        other.m_IsValid = false;
        other.m_ComputeShader = nullptr;
        return *this;
    }

    ComputeShader::~ComputeShader()
    {
        Release();
    }

    void ComputeShader::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_ComputeShader->Release();
        m_ComputeShader = nullptr;

        m_IsValid = false;
    }

    void ComputeShader::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_ComputeShader->ReleaseImmediate();
        m_ComputeShader = nullptr;

        m_IsValid = false;
    }
} // namespace Hazel