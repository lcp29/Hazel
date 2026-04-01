//
// Created by helmholtz on 2026/4/1.
//

#pragma once
#include "Hazel/RHI/RHI.h"
#include "Hazel/Core/UUID.h"
#include <string>

namespace Hazel
{
    class Sampler;
    class Texture;
    class Shader;
    class MaterialAsset;

    struct MaterialProperty
    {
        std::string name;
        uint32_t slot;
        bool isInBuffer;
        uint8_t data[64];
        RHIShaderBufferMemberReflection member;
        Texture* texture;
        Sampler* sampler;
    };

    class Material
    {
    public:
        Material() = delete;

        Material(UUID uuid, Shader* shader, MaterialAsset* asset, const std::vector<MaterialProperty>& properties)
            : m_UUID(uuid), m_Asset(asset), m_Shader(shader)
        {
            for (const auto& property : properties)
            {
                m_Properties[property.name] = property;
            }
        }

        void SetMaterialID(uint32_t id)
        {
            m_MaterialID = id;
        }

        uint32_t GetMaterialID() const
        {
            return m_MaterialID;
        }

        auto GetAsset() const
        {
            return m_Asset;
        }

        void Release() {}
        void ReleaseImmediate() {}

    private:
        UUID m_UUID = 0;
        uint32_t m_MaterialID = 0;
        Shader* m_Shader = nullptr;
        MaterialAsset* m_Asset = nullptr;
        std::unordered_map<std::string, MaterialProperty> m_Properties;
    };
} // Hazel