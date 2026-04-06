//
// Created by helmholtz on 2026/3/24.
//

#include "RenderTextureAsset.h"

#include "Hazel/Renderer/Renderer.h"

#include <unordered_map>

namespace Hazel
{
    namespace
    {
        const std::unordered_map<std::string, RHIFormat>& GetFormatMap()
        {
            static const std::unordered_map<std::string, RHIFormat> s_FormatMap = {
                {"Undefined", RHIFormat::Undefined},
                {"R8UNorm", RHIFormat::R8UNorm},
                {"R32SInt", RHIFormat::R32SInt},
                {"RG8UNorm", RHIFormat::RG8UNorm},
                {"R32SFloat", RHIFormat::R32SFloat},
                {"RG32SFloat", RHIFormat::RG32SFloat},
                {"RGB32SFloat", RHIFormat::RGB32SFloat},
                {"RG16UNorm", RHIFormat::RG16UNorm},
                {"BGRA8UNorm", RHIFormat::BGRA8UNorm},
                {"BGRA8SRGB", RHIFormat::BGRA8SRGB},
                {"RGBA8UNorm", RHIFormat::RGBA8UNorm},
                {"RGBA8SRGB", RHIFormat::RGBA8SRGB},
                {"RGB10A2UNorm", RHIFormat::RGB10A2UNorm},
                {"RGBA16SFloat", RHIFormat::RGBA16SFloat},
                {"D32SFloat", RHIFormat::D32SFloat},
                {"D32SFloatS8Uint", RHIFormat::D32SFloatS8Uint},
                {"S8Uint", RHIFormat::S8Uint}
            };
            return s_FormatMap;
        }

        const std::unordered_map<std::string, RHIImageUsageFlagBits>& GetImageUsageMap()
        {
            static const std::unordered_map<std::string, RHIImageUsageFlagBits> s_UsageMap = {
                {"TransferSource", RHIImageUsageFlagBits::TransferSource},
                {"TransferDestination", RHIImageUsageFlagBits::TransferDestination},
                {"Sampled", RHIImageUsageFlagBits::Sampled},
                {"Storage", RHIImageUsageFlagBits::Storage},
                {"ColorAttachment", RHIImageUsageFlagBits::ColorAttachment},
                {"DepthStencilAttachment", RHIImageUsageFlagBits::DepthStencilAttachment}
            };
            return s_UsageMap;
        }

        std::string FormatToString(const RHIFormat format)
        {
            for (const auto& [name, mappedFormat] : GetFormatMap())
            {
                if (mappedFormat == format)
                {
                    return name;
                }
            }

            return "Undefined";
        }

        RHIFormat TryParseFormat(const YAML::Node& node)
        {
            if (!node || !node.IsScalar())
            {
                return RHIFormat::BGRA8UNorm;
            }

            const auto it = GetFormatMap().find(node.as<std::string>());
            if (it == GetFormatMap().end())
            {
                return RHIFormat::BGRA8UNorm;
            }

            RHIFormat format = it->second;
            return format;
        }

        YAML::Node SerializeUsages(const RHIImageUsages usages)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (const auto& [name, flag] : GetImageUsageMap())
            {
                if (usages & flag)
                {
                    node.push_back(name);
                }
            }
            return node;
        }

        RHIImageUsages DeserializeUsages(const YAML::Node& node)
        {
            if (!node || !node.IsSequence())
            {
                return {};
            }

            RHIImageUsages usages = {};
            for (const auto& usageNode : node)
            {
                if (!usageNode.IsScalar())
                {
                    continue;
                }

                const auto it = GetImageUsageMap().find(usageNode.as<std::string>());
                if (it == GetImageUsageMap().end())
                {
                    continue;
                }

                usages |= it->second;
            }

            return usages;
        }
    } // namespace

    YAML::Node RenderTextureAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(m_UUID);
        rootNode["Version"] = m_Version;

        YAML::Node descNode;
        rootNode["Desc"] = descNode;

        descNode["Width"] = m_Desc.width;
        descNode["Height"] = m_Desc.height;
        descNode["Depth"] = m_Desc.depth;
        descNode["ArrayLayers"] = m_Desc.arrayLayers;
        descNode["ViewType"] = static_cast<int>(m_Desc.viewType);
        descNode["UseMipmap"] = m_Desc.useMipmap;
        descNode["PerFrame"] = m_Desc.perFrame;
        descNode["Format"] = FormatToString(m_Desc.format);
        descNode["Usages"] = SerializeUsages(m_Desc.usages);

        return rootNode;
    }

    RenderTextureAssetMeta RenderTextureAssetMeta::Deserialize(const YAML::Node& node)
    {
        RenderTextureAssetMeta meta;

        meta.m_UUID = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        meta.m_Version = node["Version"] ? node["Version"].as<uint64_t>() : 0;
        if (node["Desc"])
        {
            const auto& descNode = node["Desc"];
            meta.m_Desc.width = descNode["Width"] ? descNode["Width"].as<uint32_t>() : 256;
            meta.m_Desc.height = descNode["Height"] ? descNode["Height"].as<uint32_t>() : 256;
            meta.m_Desc.depth = descNode["Depth"] ? descNode["Depth"].as<uint32_t>() : 1;
            meta.m_Desc.arrayLayers = descNode["ArrayLayers"] ? descNode["ArrayLayers"].as<uint32_t>() : 1;
            meta.m_Desc.viewType = descNode["ViewType"]
                                     ? static_cast<RHIImageViewType>(descNode["ViewType"].as<int>())
                                     : Image2D;
            meta.m_Desc.useMipmap = descNode["UseMipmap"] ? descNode["UseMipmap"].as<bool>() : false;
            meta.m_Desc.perFrame = descNode["PerFrame"] ? descNode["PerFrame"].as<bool>() : true;
            meta.m_Desc.format = descNode["Format"] ? TryParseFormat(descNode["Format"]) : RHIFormat::BGRA8UNorm;
            meta.m_Desc.usages = descNode["Usages"] ? DeserializeUsages(descNode["Usages"]) : RHIImageUsages{};
        }
        return meta;
    }

    RenderTextureAssetMeta RenderTextureAssetMeta::CreateDefault()
    {
        RenderTextureAssetMeta meta;
        meta.m_UUID = UUID();
        meta.m_Version = 0;
        meta.m_Desc.width = 1;
        meta.m_Desc.height = 1;
        meta.m_Desc.depth = 1;
        meta.m_Desc.arrayLayers = 1;
        meta.m_Desc.viewType = RHIImageViewType::Image2D;
        meta.m_Desc.useMipmap = false;
        meta.m_Desc.perFrame = false;
        meta.m_Desc.format = RHIFormat::BGRA8UNorm;
        meta.m_Desc.usages = {};
        return meta;
    }
} // namespace Hazel
