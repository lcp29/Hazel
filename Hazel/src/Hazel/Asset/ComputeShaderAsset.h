//
// Created by helmholtz on 2026/3/29.
//

#pragma once

#include "Asset.h"
#include "Hazel/RHI/RHIShader.h"

#include <yaml-cpp/yaml.h>

namespace Hazel
{
    struct ComputeShaderAssetMeta
    {
        YAML::Node Serialize() const;
        static ComputeShaderAssetMeta Deserialize(const YAML::Node& node);
        static ComputeShaderAssetMeta CreateDefault();

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

    struct ComputeShaderAssetData
    {
        std::vector<uint32_t> binary;
        RHIShaderReflection reflection;
    };

    class ComputeShaderAsset : public Asset
    {
    public:
        ComputeShaderAsset() = delete;

        ComputeShaderAsset(AssetRegistryTerm* registryTerm,
                           const ComputeShaderAssetMeta& meta,
                           ComputeShaderAssetData data)
            : Asset(registryTerm), m_Meta(meta), m_Data(std::move(data))
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

        const ComputeShaderAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        ComputeShaderAssetMeta& GetMeta()
        {
            return m_Meta;
        }

        const ComputeShaderAssetData& GetData() const
        {
            return m_Data;
        }

        ComputeShaderAssetData& GetData()
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
        ComputeShaderAssetMeta m_Meta{};
        ComputeShaderAssetData m_Data{};
    };
} // namespace Hazel