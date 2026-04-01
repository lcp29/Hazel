//
// Created by helmholtz on 2026/3/31.
//

#pragma once

#include "Material.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class Shader
    {
    public:
        Shader() = delete;

        Shader(UUID uuid, RHIShader* vertexShader, RHIShader* fragmentShader);;
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

        Material* GetMaterial(uint32_t materialID) const
        {
            if (materialID >= m_Materials.size())
            {
                return nullptr;
            }
            return m_Materials[materialID].get();
        }

        uint32_t RegisterMaterial(std::unique_ptr<Material> material);
        void UnregisterMaterial(uint32_t materialID);

        void Release();
        void ReleaseImmediate();

    private:
        bool m_IsValid = false;
        UUID m_UUID = 0;
        RHIShader* m_VertexShader = nullptr;
        RHIShader* m_FragmentShader = nullptr;
        std::vector<std::unique_ptr<Material>> m_Materials;
        std::vector<uint32_t> m_MaterialListFreeList;
    };
} // namespace Hazel
