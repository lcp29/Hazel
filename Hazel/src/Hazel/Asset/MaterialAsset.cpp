//
// Created by helmholtz on 2026/4/1.
//

#include "MaterialAsset.h"

#include "AssetManager.h"
#include "ShaderAsset.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    namespace
    {
        const std::unordered_map<MaterialAssetPropertyType, std::string> s_MaterialAssetPropertyTypeToStringMap =
        {
            {MaterialAssetPropertyType::Int, "Int"},
            {MaterialAssetPropertyType::UInt, "UInt"},
            {MaterialAssetPropertyType::Float, "Float"},
            {MaterialAssetPropertyType::Vec2, "Vec2"},
            {MaterialAssetPropertyType::Vec3, "Vec3"},
            {MaterialAssetPropertyType::Vec4, "Vec4"},
            {MaterialAssetPropertyType::Mat3, "Mat3"},
            {MaterialAssetPropertyType::Mat4, "Mat4"},
            {MaterialAssetPropertyType::Sampler, "Sampler"},
            {MaterialAssetPropertyType::Texture, "Texture"},
            {MaterialAssetPropertyType::SamplerWithTexture, "Combined"}
        };

        std::string MaterialAssetPropertyTypeToString(MaterialAssetPropertyType type)
        {
            return s_MaterialAssetPropertyTypeToStringMap.at(type);
        }

        MaterialAssetPropertyType StringToMaterialAssetPropertyType(const std::string& str)
        {
            for (const auto& [type, name] : s_MaterialAssetPropertyTypeToStringMap)
            {
                if (name == str)
                {
                    return type;
                }
            }
            return MaterialAssetPropertyType::Int;
        }

        YAML::Node SerializeColorBlendAttachment(const RHIColorBlendAttachmentDesc& attachment)
        {
            YAML::Node node;
            node["BlendEnable"] = attachment.blendEnable;
            node["SrcColorBlendFactor"] = static_cast<uint32_t>(attachment.srcColorBlendFactor);
            node["DstColorBlendFactor"] = static_cast<uint32_t>(attachment.dstColorBlendFactor);
            node["ColorBlendOp"] = static_cast<uint32_t>(attachment.colorBlendOp);
            node["SrcAlphaBlendFactor"] = static_cast<uint32_t>(attachment.srcAlphaBlendFactor);
            node["DstAlphaBlendFactor"] = static_cast<uint32_t>(attachment.dstAlphaBlendFactor);
            node["AlphaBlendOp"] = static_cast<uint32_t>(attachment.alphaBlendOp);
            node["ColorWriteMask"] = static_cast<uint8_t>(attachment.colorWriteMask);
            return node;
        }

        RHIColorBlendAttachmentDesc DeserializeColorBlendAttachment(const YAML::Node& node)
        {
            RHIColorBlendAttachmentDesc attachment{};
            if (!node)
            {
                return attachment;
            }

            attachment.blendEnable = node["BlendEnable"] ? node["BlendEnable"].as<bool>() : attachment.blendEnable;
            attachment.srcColorBlendFactor = node["SrcColorBlendFactor"]
                                                 ? static_cast<RHIBlendFactor>(node["SrcColorBlendFactor"].as<
                                                     uint32_t>())
                                                 : attachment.srcColorBlendFactor;
            attachment.dstColorBlendFactor = node["DstColorBlendFactor"]
                                                 ? static_cast<RHIBlendFactor>(node["DstColorBlendFactor"].as<
                                                     uint32_t>())
                                                 : attachment.dstColorBlendFactor;
            attachment.colorBlendOp = node["ColorBlendOp"]
                                          ? static_cast<RHIBlendOp>(node["ColorBlendOp"].as<uint32_t>())
                                          : attachment.colorBlendOp;
            attachment.srcAlphaBlendFactor = node["SrcAlphaBlendFactor"]
                                                 ? static_cast<RHIBlendFactor>(node["SrcAlphaBlendFactor"].as<
                                                     uint32_t>())
                                                 : attachment.srcAlphaBlendFactor;
            attachment.dstAlphaBlendFactor = node["DstAlphaBlendFactor"]
                                                 ? static_cast<RHIBlendFactor>(node["DstAlphaBlendFactor"].as<
                                                     uint32_t>())
                                                 : attachment.dstAlphaBlendFactor;
            attachment.alphaBlendOp = node["AlphaBlendOp"]
                                          ? static_cast<RHIBlendOp>(node["AlphaBlendOp"].as<uint32_t>())
                                          : attachment.alphaBlendOp;
            attachment.colorWriteMask = node["ColorWriteMask"]
                                            ? RHIColorComponentFlags(node["ColorWriteMask"].as<uint8_t>())
                                            : attachment.colorWriteMask;
            return attachment;
        }

        YAML::Node SerializePipelineState(const MaterialPipelineState& state)
        {
            YAML::Node node;
            node["PolygonMode"] = static_cast<uint32_t>(state.polygonMode);
            node["CullMode"] = static_cast<uint32_t>(state.cullMode);
            node["DepthClampEnable"] = state.depthClampEnable;
            node["DepthBiasEnable"] = state.depthBiasEnable;
            node["DepthTestEnable"] = state.depthTestEnable;
            node["DepthWriteEnable"] = state.depthWriteEnable;
            node["DepthCompareOp"] = static_cast<uint32_t>(state.depthCompareOp);
            node["StencilTestEnable"] = state.stencilTestEnable;

            YAML::Node colorBlendAttachmentsNode;
            for (const auto& attachment : state.colorBlendAttachments)
            {
                colorBlendAttachmentsNode.push_back(SerializeColorBlendAttachment(attachment));
            }
            node["ColorBlendAttachments"] = colorBlendAttachmentsNode;
            return node;
        }

        MaterialPipelineState DeserializePipelineState(const YAML::Node& node)
        {
            MaterialPipelineState state{};
            if (!node)
            {
                return state;
            }

            state.polygonMode = node["PolygonMode"]
                                    ? static_cast<RHIPolygonMode>(node["PolygonMode"].as<uint32_t>())
                                    : state.polygonMode;
            state.cullMode = node["CullMode"]
                                 ? static_cast<RHICullMode>(node["CullMode"].as<uint32_t>())
                                 : state.cullMode;
            state.depthClampEnable = node["DepthClampEnable"]
                                         ? node["DepthClampEnable"].as<bool>()
                                         : state.depthClampEnable;
            state.depthBiasEnable = node["DepthBiasEnable"]
                                        ? node["DepthBiasEnable"].as<bool>()
                                        : state.depthBiasEnable;
            state.depthTestEnable = node["DepthTestEnable"]
                                        ? node["DepthTestEnable"].as<bool>()
                                        : state.depthTestEnable;
            state.depthWriteEnable = node["DepthWriteEnable"]
                                         ? node["DepthWriteEnable"].as<bool>()
                                         : state.depthWriteEnable;
            state.depthCompareOp = node["DepthCompareOp"]
                                       ? static_cast<RHICompareOp>(node["DepthCompareOp"].as<uint32_t>())
                                       : state.depthCompareOp;
            state.stencilTestEnable = node["StencilTestEnable"]
                                          ? node["StencilTestEnable"].as<bool>()
                                          : state.stencilTestEnable;
            if (node["ColorBlendAttachments"])
            {
                for (const auto& attachmentNode : node["ColorBlendAttachments"])
                {
                    state.colorBlendAttachments.push_back(DeserializeColorBlendAttachment(attachmentNode));
                }
            }
            return state;
        }
    }

    YAML::Node MaterialAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(m_UUID);
        rootNode["Shader"] = static_cast<uint64_t>(m_Shader);
        rootNode["Version"] = m_Version;
        rootNode["PipelineState"] = SerializePipelineState(m_PipelineState);

        YAML::Node propertiesNode;
        for (const auto& property : m_Properties)
        {
            YAML::Node propertyNode;
            propertyNode["Name"] = property.name;
            propertyNode["Type"] = MaterialAssetPropertyTypeToString(property.type);
            propertyNode["Data"] = YAML::Binary(property.data, 64);
            propertyNode["Sampler"] = static_cast<uint64_t>(property.sampler);
            propertyNode["Texture"] = static_cast<uint64_t>(property.texture);
            propertiesNode.push_back(propertyNode);
        }
        rootNode["Properties"] = propertiesNode;

        return rootNode;
    }

    MaterialAssetMeta MaterialAssetMeta::Deserialize(const YAML::Node& node)
    {
        MaterialAssetMeta meta;

        meta.m_UUID = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        meta.m_Version = node["Version"] ? node["Version"].as<uint64_t>() : 0;
        meta.m_PipelineState = DeserializePipelineState(node["PipelineState"]);

        if (!node["Shader"])
        {
            meta.m_Shader = UUID(-1);
            return meta;
        }

        meta.m_Shader = UUID(node["Shader"].as<uint64_t>());

        if (node["Properties"])
        {
            for (const auto& propertyNode : node["Properties"])
            {
                MaterialAssetProperty prop;

                prop.name = propertyNode["Name"] ? propertyNode["Name"].as<std::string>() : "null";
                prop.type = propertyNode["Type"]
                                ? StringToMaterialAssetPropertyType(propertyNode["Type"].as<std::string>())
                                : MaterialAssetPropertyType::Int;
                if (propertyNode["Data"])
                {
                    auto data = propertyNode["Data"].as<YAML::Binary>();
                    auto dataSize = data.size();
                    std::copy_n(data.data(), std::min(dataSize, static_cast<size_t>(64)), prop.data);
                }
                prop.sampler = propertyNode["Sampler"] ? UUID(propertyNode["Sampler"].as<uint64_t>()) : UUID(-1);
                prop.texture = propertyNode["Texture"] ? UUID(propertyNode["Texture"].as<uint64_t>()) : UUID(-1);

                meta.m_Properties.push_back(prop);
            }
        }
        return meta;
    }

    MaterialAssetMeta MaterialAssetMeta::CreateDefault()
    {
        MaterialAssetMeta meta;
        meta.m_UUID = UUID();
        meta.m_Shader = UUID(-1);
        meta.m_PipelineState = {};
        meta.m_Properties = {};
        meta.m_Version = 0;
        return meta;
    }

    void MaterialAssetMeta::RefreshShader(AssetManager* assetManager)
    {
        if (m_Shader == UUID(-1))
        {
            ClearProperties();
            VersionUp();
            return;
        }

        ShaderAsset* shaderAsset = static_cast<ShaderAsset*>(assetManager->RequestAssetBlocked(m_Shader));
        std::vector<MaterialAssetProperty> oldProperties = std::move(m_Properties);
        ClearProperties();

        auto& reflection = shaderAsset->GetReflection();
        for (auto& set : reflection.resourceGroups)
        {
            // only reflect material property
            if (set.set != 2)
            {
                continue;
            }
            for (auto& slot : set.slots)
            {
                if (slot.slot != 0)
                {
                    continue;
                }

                for (auto& member : slot.buffer.members)
                {
                    bool propertyExists = false;
                    for (auto& metaProperty : oldProperties)
                    {
                        if (metaProperty.name == member.name)
                        {
                            propertyExists = true;
                            AddProperty(metaProperty);
                            break;
                        }
                    }
                    if (!propertyExists)
                    {
                        MaterialAssetProperty prop;
                        prop.name = member.name;
                        std::ranges::fill(prop.data, 0);
                        prop.sampler = UUID(-1);
                        prop.texture = UUID(-1);
                        prop.slot = slot.slot;
                        prop.member = member;

                        bool isSampler = member.name.starts_with("sampler_");
                        bool isTexture = member.name.starts_with("texture_");
                        bool isCombined = member.name.starts_with("combined_");
                        bool isValue = !(isSampler || isTexture || isCombined);

                        if (isValue)
                        {
                            switch (member.baseType)
                            {
                                case RHIShaderValueBaseType::SInt:
                                    prop.type = MaterialAssetPropertyType::Int;
                                    break;
                                case RHIShaderValueBaseType::UInt:
                                    prop.type = MaterialAssetPropertyType::UInt;
                                    break;
                                case RHIShaderValueBaseType::Float:
                                {
                                    if (member.columns == 1)
                                    {
                                        switch (member.rows)
                                        {
                                            case 1:
                                                prop.type = MaterialAssetPropertyType::Float;
                                                break;
                                            case 2:
                                                prop.type = MaterialAssetPropertyType::Vec2;
                                                break;
                                            case 3:
                                                prop.type = MaterialAssetPropertyType::Vec3;
                                                break;
                                            case 4:
                                                prop.type = MaterialAssetPropertyType::Vec4;
                                                break;
                                            default:
                                                break;
                                        }
                                    }
                                    else
                                    {
                                        switch (member.columns)
                                        {
                                            case 3:
                                                prop.type = MaterialAssetPropertyType::Mat3;
                                                break;
                                            case 4:
                                                prop.type = MaterialAssetPropertyType::Mat4;
                                                break;
                                            default:
                                                break;
                                        }
                                    }
                                }
                                default:
                                    break;
                            }
                        }

                        if (isSampler)
                        {
                            prop.type = MaterialAssetPropertyType::Sampler;
                        }

                        if (isTexture)
                        {
                            prop.type = MaterialAssetPropertyType::Texture;
                        }

                        if (isCombined)
                        {
                            prop.type = MaterialAssetPropertyType::SamplerWithTexture;
                        }

                        AddProperty(prop);
                    }
                }
            }
        }
        VersionUp();
    }
} // Hazel