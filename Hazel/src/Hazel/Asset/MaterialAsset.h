//
// Created by helmholtz on 2026/4/1.
//

#pragma once
#include "Asset.h"
#include "Hazel/RHI/RHI.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class ShaderAsset;
    class SamplerAsset;
    class TextureAsset;
    class AssetManager;

    enum class MaterialAssetPropertyType
    {
        Int,
        UInt,
        Float,
        Vec2,
        Vec3,
        Vec4,
        Mat3,
        Mat4,
        Sampler,
        Texture,
        SamplerWithTexture
    };

    struct MaterialAssetProperty
    {
        std::string name{};
        MaterialAssetPropertyType type = MaterialAssetPropertyType::Float;
        uint8_t data[64];

        union
        {
            UUID sampler;
            uint64_t bindlessID = 0;
        };

        UUID texture{};
        uint32_t slot = 0;
        RHIShaderBufferMemberReflection member{};
    };

    struct MaterialPipelineState
    {
        RHIPolygonMode polygonMode = RHIPolygonMode::Fill;
        RHICullMode cullMode = RHICullMode::Back;
        bool depthClampEnable = false;
        bool depthBiasEnable = false;
        bool depthTestEnable = false;
        bool depthWriteEnable = false;
        RHICompareOp depthCompareOp = RHICompareOp::LessOrEqual;
        bool stencilTestEnable = false;

        bool operator==(const MaterialPipelineState& other) const
        {
            return polygonMode == other.polygonMode && cullMode == other.cullMode
                   && depthClampEnable == other.depthClampEnable && depthBiasEnable == other.depthBiasEnable
                   && depthTestEnable == other.depthTestEnable && depthWriteEnable == other.depthWriteEnable
                   && depthCompareOp == other.depthCompareOp && stencilTestEnable == other.stencilTestEnable;
        };
    };

    struct MaterialAssetMeta
    {
        YAML::Node Serialize() const;
        static MaterialAssetMeta Deserialize(const YAML::Node& node);
        static MaterialAssetMeta CreateDefault();

        UUID GetUUID() const { return m_UUID; }

        uint64_t GetVersion() const { return m_Version; }

        void VersionUp() { m_Version++; }

        UUID GetShader() const { return m_Shader; }

        void SetShader(UUID shader)
        {
            if (m_Shader == shader) { return; }
            m_Shader = shader;
            VersionUp();
        }

        const std::vector<MaterialAssetProperty>& GetProperties() const { return m_Properties; }

        const MaterialPipelineState& GetPipelineState() const { return m_PipelineState; }

        void SetPipelineState(MaterialPipelineState pipelineState)
        {
            if (m_PipelineState == pipelineState) { return; }
            m_PipelineState = std::move(pipelineState);
            VersionUp();
        }

        void ClearProperties()
        {
            if (m_Properties.empty()) { return; }
            m_Properties.clear();
            VersionUp();
        }

        void ReserveProperties(size_t count) { m_Properties.reserve(count); }

        void AddProperty(const MaterialAssetProperty& property)
        {
            m_Properties.push_back(property);
            VersionUp();
        }

        void SetProperties(std::vector<MaterialAssetProperty> properties)
        {
            if (m_Properties.size() == properties.size()
                && std::equal(m_Properties.begin(),
                              m_Properties.end(),
                              properties.begin(),
                              [](const MaterialAssetProperty& lhs, const MaterialAssetProperty& rhs) {
                                  return lhs.name == rhs.name && lhs.type == rhs.type
                                         && std::memcmp(lhs.data, rhs.data, sizeof(lhs.data)) == 0
                                         && lhs.sampler == rhs.sampler && lhs.texture == rhs.texture;
                              }))
            {
                return;
            }
            m_Properties = std::move(properties);
            VersionUp();
        }

        void SetPropertyData(size_t index, const void* data, size_t size)
        {
            if (index >= m_Properties.size()) { return; }

            const auto copySize = std::min(size, sizeof(m_Properties[index].data));
            if (std::memcmp(m_Properties[index].data, data, copySize) == 0) { return; }

            std::memcpy(m_Properties[index].data, data, copySize);
            if (copySize < sizeof(m_Properties[index].data))
            {
                std::memset(m_Properties[index].data + copySize, 0, sizeof(m_Properties[index].data) - copySize);
            }
            VersionUp();
        }

        void SetPropertySampler(size_t index, UUID sampler)
        {
            if (index >= m_Properties.size() || m_Properties[index].sampler == sampler) { return; }
            m_Properties[index].sampler = sampler;
            VersionUp();
        }

        void SetPropertyTexture(size_t index, UUID texture)
        {
            if (index >= m_Properties.size() || m_Properties[index].texture == texture) { return; }
            m_Properties[index].texture = texture;
            VersionUp();
        }

        void RefreshShader(AssetManager* assetManager);

      private:
        UUID m_UUID = 0;
        UUID m_Shader = UUID(-1);
        MaterialPipelineState m_PipelineState{};
        std::vector<MaterialAssetProperty> m_Properties;
        uint64_t m_Version = 0;
    };

    class MaterialAsset : public Asset
    {
      public:
        MaterialAsset() = delete;

        MaterialAsset(AssetRegistryTerm* registryTerm, const MaterialAssetMeta& meta)
            : Asset(registryTerm)
            , m_Meta(meta)
        {}

        uint64_t GetVersion() const final { return m_Meta.GetVersion(); }

        void VersionUp() final { m_Meta.VersionUp(); }

        const MaterialAssetMeta& GetMeta() const { return m_Meta; }

        MaterialAssetMeta& GetMeta() { return m_Meta; }

        UUID GetShader() const { return m_Meta.GetShader(); }

        MaterialPipelineState GetPipelineState() const { return m_Meta.GetPipelineState(); }

      private:
        MaterialAssetMeta m_Meta{};
    };
} // namespace Hazel