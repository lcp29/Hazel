//
// Created by helmholtz on 2026/3/25.
//

#include "SamplerAsset.h"

#include "Hazel/RHI/RHIPipeline.h"
#include "Hazel/Renderer/Renderer.h"

#include <unordered_map>

namespace Hazel
{
    namespace
    {
        const std::unordered_map<std::string, RHISamplerFilter>& GetFilterMap()
        {
            static const std::unordered_map<std::string, RHISamplerFilter> s_FilterMap = {
                {"Nearest", RHISamplerFilter::Nearest},
                {"Linear", RHISamplerFilter::Linear}
            };
            return s_FilterMap;
        }

        const std::unordered_map<std::string, RHISamplerAddressMode>& GetAddressModeMap()
        {
            static const std::unordered_map<std::string, RHISamplerAddressMode> s_AddressModeMap = {
                {"Repeat", RHISamplerAddressMode::Repeat},
                {"MirroredRepeat", RHISamplerAddressMode::MirroredRepeat},
                {"ClampToEdge", RHISamplerAddressMode::ClampToEdge},
                {"ClampToBorder", RHISamplerAddressMode::ClampToBorder}
            };
            return s_AddressModeMap;
        }

        const std::unordered_map<std::string, RHICompareOp>& GetCompareOpMap()
        {
            static const std::unordered_map<std::string, RHICompareOp> s_CompareOpMap = {
                {"Never", RHICompareOp::Never},
                {"Less", RHICompareOp::Less},
                {"Equal", RHICompareOp::Equal},
                {"LessOrEqual", RHICompareOp::LessOrEqual},
                {"Greater", RHICompareOp::Greater},
                {"NotEqual", RHICompareOp::NotEqual},
                {"GreaterOrEqual", RHICompareOp::GreaterOrEqual},
                {"Always", RHICompareOp::Always}
            };
            return s_CompareOpMap;
        }

        template <typename EnumType>
        std::string EnumToString(const EnumType value, const std::unordered_map<std::string, EnumType>& map)
        {
            for (const auto& [name, mappedValue] : map)
            {
                if (mappedValue == value)
                {
                    return name;
                }
            }

            return {};
        }

        template <typename EnumType>
        EnumType TryParseEnum(const YAML::Node& node,
                              const std::unordered_map<std::string, EnumType>& map)
        {
            const auto it = map.find(node.as<std::string>());
            if (it == map.end())
            {
                return (*map.begin()).second;
            }
            return it->second;
        }
    } // namespace

    YAML::Node SamplerAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(m_UUID);
        rootNode["Version"] = m_Version;

        YAML::Node samplerDescNode;
        rootNode["Desc"] = samplerDescNode;
        samplerDescNode["MinFilter"] = EnumToString(m_Desc.minFilter, GetFilterMap());
        samplerDescNode["MagFilter"] = EnumToString(m_Desc.magFilter, GetFilterMap());
        samplerDescNode["MipFilter"] = EnumToString(m_Desc.mipFilter, GetFilterMap());
        samplerDescNode["AddressModeU"] = EnumToString(m_Desc.addressModeU, GetAddressModeMap());
        samplerDescNode["AddressModeV"] = EnumToString(m_Desc.addressModeV, GetAddressModeMap());
        samplerDescNode["AddressModeW"] = EnumToString(m_Desc.addressModeW, GetAddressModeMap());
        samplerDescNode["MipLodBias"] = m_Desc.mipLodBias;
        samplerDescNode["MinLod"] = m_Desc.minLod;
        samplerDescNode["MaxLod"] = m_Desc.maxLod;
        samplerDescNode["MaxAnisotropy"] = m_Desc.maxAnisotropy;
        samplerDescNode["EnableAnisotropy"] = m_Desc.enableAnisotropy;
        samplerDescNode["CompareEnable"] = m_Desc.compareEnable;
        samplerDescNode["CompareOp"] = EnumToString(m_Desc.compareOp, GetCompareOpMap());

        return rootNode;
    }

    SamplerAssetMeta SamplerAssetMeta::Deserialize(const YAML::Node& node)
    {
        SamplerAssetMeta meta;
        meta.m_UUID = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        meta.m_Version = node["Version"] ? node["Version"].as<uint64_t>() : 0;

        auto& desc = meta.m_Desc;
        auto descNode = node["Desc"];
        desc.minFilter = descNode["MinFilter"]
                             ? TryParseEnum(descNode["MinFilter"], GetFilterMap())
                             : RHISamplerFilter::Linear;
        desc.magFilter = descNode["MagFilter"]
                             ? TryParseEnum(descNode["MagFilter"], GetFilterMap())
                             : RHISamplerFilter::Linear;
        desc.mipFilter = descNode["MipFilter"]
                             ? TryParseEnum(descNode["MipFilter"], GetFilterMap())
                             : RHISamplerFilter::Linear;
        desc.addressModeU = descNode["AddressModeU"]
                                ? TryParseEnum(descNode["AddressModeU"], GetAddressModeMap())
                                : RHISamplerAddressMode::Repeat;
        desc.addressModeV = descNode["AddressModeV"]
                                ? TryParseEnum(descNode["AddressModeV"], GetAddressModeMap())
                                : RHISamplerAddressMode::Repeat;
        desc.addressModeW = descNode["AddressModeW"]
                                ? TryParseEnum(descNode["AddressModeW"], GetAddressModeMap())
                                : RHISamplerAddressMode::Repeat;
        desc.mipLodBias = descNode["MipLodBias"] ? descNode["MipLodBias"].as<float>() : 0.0f;
        desc.minLod = descNode["MinLod"] ? descNode["MinLod"].as<float>() : 0.0f;
        desc.maxLod = descNode["MaxLod"] ? descNode["MaxLod"].as<float>() : 0.0f;
        desc.maxAnisotropy = descNode["MaxAnisotropy"] ? descNode["MaxAnisotropy"].as<float>() : 0.0f;
        desc.enableAnisotropy = descNode["EnableAnisotropy"] ? descNode["EnableAnisotropy"].as<bool>() : false;
        desc.compareEnable = descNode["CompareEnable"] ? descNode["CompareEnable"].as<bool>() : false;
        desc.compareOp = descNode["CompareOp"]
                             ? TryParseEnum(descNode["CompareOp"], GetCompareOpMap())
                             : RHICompareOp::Never;

        return meta;
    }

    SamplerAssetMeta SamplerAssetMeta::CreateDefault()
    {
        SamplerAssetMeta meta;
        meta.m_UUID = UUID();
        meta.m_Version = 0;
        meta.m_Desc.minFilter = RHISamplerFilter::Linear;
        meta.m_Desc.magFilter = RHISamplerFilter::Linear;
        meta.m_Desc.mipFilter = RHISamplerFilter::Linear;
        meta.m_Desc.addressModeU = RHISamplerAddressMode::Repeat;
        meta.m_Desc.addressModeV = RHISamplerAddressMode::Repeat;
        meta.m_Desc.addressModeW = RHISamplerAddressMode::Repeat;
        meta.m_Desc.mipLodBias = 0.0f;
        meta.m_Desc.minLod = 0.0f;
        meta.m_Desc.maxLod = 0.0f;
        meta.m_Desc.maxAnisotropy = 0.0f;
        meta.m_Desc.enableAnisotropy = false;
        meta.m_Desc.compareEnable = false;
        meta.m_Desc.compareOp = RHICompareOp::Never;
        return meta;
    }
} // namespace Hazel
