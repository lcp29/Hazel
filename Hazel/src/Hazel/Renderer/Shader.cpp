//
// Created by helmholtz on 2026/3/31.
//

#include "Shader.h"

#include "Hazel/Asset/MaterialAsset.h"

namespace Hazel
{
    Shader::Shader(UUID uuid, RHIShader* vertexShader, RHIShader* fragmentShader)
        : m_IsValid(true), m_UUID(uuid), m_VertexShader(vertexShader), m_FragmentShader(fragmentShader) {}

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

    uint32_t Shader::RegisterMaterial(std::unique_ptr<Material> material)
    {
        if (!m_MaterialListFreeList.empty())
        {
            uint32_t materialID = m_MaterialListFreeList.back();
            m_MaterialListFreeList.pop_back();
            material->SetMaterialID(materialID);
            m_Materials[materialID] = std::move(material);
            return materialID;
        }
        auto materialID = m_Materials.size();
        material->SetMaterialID(materialID);
        m_Materials.push_back(std::move(material));
        return materialID;
    }

    void Shader::UnregisterMaterial(uint32_t materialID)
    {
        m_Materials[materialID].reset();
        m_MaterialListFreeList.push_back(materialID);
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

        for (auto& material : m_Materials)
        {
            if (material)
            {
                if (auto* asset = material->GetAsset())
                {
                    asset->Unload();
                }
            }
        }

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

        for (auto& material : m_Materials)
        {
            if (material)
            {
                if (auto* asset = material->GetAsset())
                {
                    asset->Unload();
                }
            }
        }

        m_IsValid = false;
    }
} // namespace Hazel