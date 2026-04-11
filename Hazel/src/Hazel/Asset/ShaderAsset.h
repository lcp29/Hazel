//
// Created by helmholtz on 2026/3/31.
//

#pragma once

#include "Asset.h"
#include "Hazel/RHI/RHIShader.h"

#include <yaml-cpp/yaml.h>

namespace Hazel
{
    struct ShaderAssetMeta
    {
        YAML::Node Serialize() const;
        static ShaderAssetMeta Deserialize(const YAML::Node& node);
        static ShaderAssetMeta CreateDefault();

        UUID GetUUID() const
        {
            return m_UUID;
        }

        uint64_t GetVersion() const
        {
            return m_Version;
        }

        void VersionUp()
        {
            m_Version++;
        }

    private:
        UUID m_UUID = 0;
        uint64_t m_Version = 0;
    };

    struct ShaderAssetData
    {
        std::vector<uint32_t> vertexBinary;
        std::vector<uint32_t> fragmentBinary;
        RHIShaderReflection reflection;
    };

    class ShaderAsset : public Asset
    {
    public:
        ShaderAsset() = delete;

        ShaderAsset(AssetRegistryTerm* registryTerm,
                    const ShaderAssetMeta& meta,
                    ShaderAssetData shaderData)
            : Asset(registryTerm), m_Meta(meta), m_Data(std::move(shaderData))
        {
        }

        uint64_t GetVersion() const final
        {
            return m_Meta.GetVersion();
        }

        void VersionUp() final
        {
            m_Meta.VersionUp();
        }

        const ShaderAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        ShaderAssetMeta& GetMeta()
        {
            return m_Meta;
        }

        const ShaderAssetData& GetData() const
        {
            return m_Data;
        }

        ShaderAssetData& GetData()
        {
            return m_Data;
        }

        const RHIShaderReflection& GetReflection() const
        {
            return m_Data.reflection;
        }

        RHIShaderReflection& GetReflection()
        {
            return m_Data.reflection;
        }

    private:
        ShaderAssetMeta m_Meta{};
        ShaderAssetData m_Data{};
    };
} // namespace Hazel