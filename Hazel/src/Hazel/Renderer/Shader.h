//
// Created by helmholtz on 2026/3/31.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class Shader
    {
    public:
        Shader() = delete;

        Shader(UUID uuid, RHIShader* vertexShader, RHIShader* fragmentShader)
            : m_IsValid(true), m_UUID(uuid), m_VertexShader(vertexShader), m_FragmentShader(fragmentShader) {};
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;
        ~Shader();

        bool IsValid() const
        {
            return m_IsValid;
        }

        RHIShader* GetVertexShader() const
        {
            return m_VertexShader;
        }

        RHIShader* GetFragmentShader() const
        {
            return m_FragmentShader;
        }

        void Release();
        void ReleaseImmediate();

    private:
        bool m_IsValid = false;
        UUID m_UUID = 0;
        RHIShader* m_VertexShader = nullptr;
        RHIShader* m_FragmentShader = nullptr;
    };
} // namespace Hazel
