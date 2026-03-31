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
        rootNode["UUID"] = static_cast<uint64_t>(uuid);

        YAML::Node descNode;
        rootNode["Desc"] = descNode;

        descNode["Width"] = desc.width;
        descNode["Height"] = desc.height;
        descNode["Depth"] = desc.depth;
        descNode["ArrayLayers"] = desc.arrayLayers;
        descNode["ViewType"] = static_cast<int>(desc.viewType);
        descNode["UseMipmap"] = desc.useMipmap;
        descNode["PerFrame"] = desc.perFrame;
        descNode["Format"] = FormatToString(desc.format);
        descNode["Usages"] = SerializeUsages(desc.usages);

        return rootNode;
    }

    RenderTextureAssetMeta RenderTextureAssetMeta::Deserialize(const YAML::Node& node)
    {
        RenderTextureAssetMeta meta;

        meta.uuid = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        if (node["Desc"])
        {
            const auto& descNode = node["Desc"];
            meta.desc.width = descNode["Width"] ? descNode["Width"].as<uint32_t>() : 256;
            meta.desc.height = descNode["Height"] ? descNode["Height"].as<uint32_t>() : 256;
            meta.desc.depth = descNode["Depth"] ? descNode["Depth"].as<uint32_t>() : 1;
            meta.desc.arrayLayers = descNode["ArrayLayers"] ? descNode["ArrayLayers"].as<uint32_t>() : 1;
            meta.desc.viewType = descNode["ViewType"]
                                     ? static_cast<RHIImageViewType>(descNode["ViewType"].as<int>())
                                     : Image2D;
            meta.desc.useMipmap = descNode["UseMipmap"] ? descNode["UseMipmap"].as<bool>() : false;
            meta.desc.perFrame = descNode["PerFrame"] ? descNode["PerFrame"].as<bool>() : true;
            meta.desc.format = descNode["Format"] ? TryParseFormat(descNode["Format"]) : RHIFormat::BGRA8UNorm;
            meta.desc.usages = descNode["Usages"] ? DeserializeUsages(descNode["Usages"]) : RHIImageUsages{};
        }
        return meta;
    }

    void RenderTextureAsset::Load()
    {
        if (m_IsLoaded)
        {
            return;
        }

        auto renderTexture = std::make_unique<RenderTexture>(m_UUID, m_Renderer, m_Meta.desc);
        m_RenderTexture = m_Renderer->AddRenderTexture(std::move(renderTexture));

        m_IsLoaded = true;
    }

    void RenderTextureAsset::Unload()
    {
        if (!m_IsLoaded)
        {
            return;
        }

        m_Renderer->RemoveRenderTexture(m_RenderTexture);
        m_RenderTexture = nullptr;

        m_IsLoaded = false;
    }

    RenderTextureAsset::RenderTextureAsset(RenderTextureAsset&& other) noexcept
        : m_UUID(other.m_UUID)
          , m_IsLoaded(other.m_IsLoaded)
          , m_MetaPath(std::move(other.m_MetaPath))
          , m_Renderer(other.m_Renderer)
          , m_Meta(other.m_Meta)
          , m_RenderTexture(other.m_RenderTexture)
    {
        other.m_IsLoaded = false;
        other.m_Renderer = nullptr;
        other.m_RenderTexture = nullptr;
    }

    RenderTextureAsset& RenderTextureAsset::operator=(RenderTextureAsset&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_UUID = other.m_UUID;
        m_IsLoaded = other.m_IsLoaded;
        m_MetaPath = std::move(other.m_MetaPath);
        m_Renderer = other.m_Renderer;
        m_Meta = other.m_Meta;
        m_RenderTexture = other.m_RenderTexture;

        other.m_IsLoaded = false;
        other.m_Renderer = nullptr;
        other.m_RenderTexture = nullptr;
        return *this;
    }

    RenderTextureAsset::~RenderTextureAsset()
    {
        Release();
    }

    void RenderTextureAsset::Release()
    {
        Unload();
    }
} // namespace Hazel