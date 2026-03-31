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
        rootNode["UUID"] = static_cast<uint64_t>(uuid);

        YAML::Node samplerDescNode;
        rootNode["Desc"] = samplerDescNode;
        samplerDescNode["MinFilter"] = EnumToString(desc.minFilter, GetFilterMap());
        samplerDescNode["MagFilter"] = EnumToString(desc.magFilter, GetFilterMap());
        samplerDescNode["MipFilter"] = EnumToString(desc.mipFilter, GetFilterMap());
        samplerDescNode["AddressModeU"] = EnumToString(desc.addressModeU, GetAddressModeMap());
        samplerDescNode["AddressModeV"] = EnumToString(desc.addressModeV, GetAddressModeMap());
        samplerDescNode["AddressModeW"] = EnumToString(desc.addressModeW, GetAddressModeMap());
        samplerDescNode["MipLodBias"] = desc.mipLodBias;
        samplerDescNode["MinLod"] = desc.minLod;
        samplerDescNode["MaxLod"] = desc.maxLod;
        samplerDescNode["MaxAnisotropy"] = desc.maxAnisotropy;
        samplerDescNode["EnableAnisotropy"] = desc.enableAnisotropy;
        samplerDescNode["CompareEnable"] = desc.compareEnable;
        samplerDescNode["CompareOp"] = EnumToString(desc.compareOp, GetCompareOpMap());

        return rootNode;
    }

    SamplerAssetMeta SamplerAssetMeta::Deserialize(const YAML::Node& node)
    {
        SamplerAssetMeta meta;
        meta.uuid = UUID(node["UUID"].as<uint64_t>());

        auto& desc = meta.desc;
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

    void SamplerAsset::Load()
    {
        if (m_IsLoaded)
        {
            return;
        }

        auto sampler = m_Renderer->GetDevice()->CreateSampler(m_Meta.desc);
        m_Sampler = m_Renderer->AddSampler(std::make_unique<Sampler>(m_UUID, m_Renderer, m_Meta.desc, sampler));

        m_IsLoaded = true;
    }

    void SamplerAsset::Unload()
    {
        if (!m_IsLoaded)
        {
            return;
        }

        m_Renderer->RemoveSampler(m_Sampler);
        m_Sampler = nullptr;
        m_IsLoaded = false;
    }

    SamplerAsset::SamplerAsset(SamplerAsset&& other) noexcept
        : m_UUID(other.m_UUID)
          , m_FilePath(std::move(other.m_FilePath))
          , m_Renderer(other.m_Renderer)
          , m_Meta(other.m_Meta)
          , m_Sampler(other.m_Sampler)
    {
        other.m_Renderer = nullptr;
        other.m_Sampler = nullptr;
    }

    SamplerAsset& SamplerAsset::operator=(SamplerAsset&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_UUID = other.m_UUID;
        m_FilePath = std::move(other.m_FilePath);
        m_Renderer = other.m_Renderer;
        m_Meta = other.m_Meta;
        m_Sampler = other.m_Sampler;

        other.m_Renderer = nullptr;
        other.m_Sampler = nullptr;
        return *this;
    }

    SamplerAsset::~SamplerAsset()
    {
        Release();
    }

    void SamplerAsset::Release()
    {
        Unload();
    }

    void SamplerAsset::Recreate()
    {
        if (m_IsLoaded)
        {
            m_Renderer->GetDevice()->WaitIdle();
            Unload();
            Load();
        }
    }
} // namespace Hazel