//
// Created by helmholtz on 2026/3/31.
//

#include "Shader.h"

namespace Hazel
{
    Shader::Shader(Shader&& other) noexcept
        : m_IsValid(other.m_IsValid),
          m_UUID(other.m_UUID),
          m_VertexShader(other.m_VertexShader),
          m_FragmentShader(other.m_FragmentShader)
    {
        other.m_IsValid = false;
        other.m_VertexShader = nullptr;
        other.m_FragmentShader = nullptr;
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_IsValid = other.m_IsValid;
        m_UUID = other.m_UUID;
        m_VertexShader = other.m_VertexShader;
        m_FragmentShader = other.m_FragmentShader;

        other.m_IsValid = false;
        other.m_VertexShader = nullptr;
        other.m_FragmentShader = nullptr;
        return *this;
    }

    Shader::~Shader()
    {
        Release();
    }

    void Shader::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_VertexShader->Release();
        m_FragmentShader->Release();
        m_VertexShader = nullptr;
        m_FragmentShader = nullptr;

        m_IsValid = false;
    }

    void Shader::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_VertexShader->ReleaseImmediate();
        m_FragmentShader->ReleaseImmediate();
        m_VertexShader = nullptr;
        m_FragmentShader = nullptr;

        m_IsValid = false;
    }
} // namespace Hazel
