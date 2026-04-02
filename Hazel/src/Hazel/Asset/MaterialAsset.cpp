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
        constexpr int kMaterialPropertyResourceGroup = 2;

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
    }

    YAML::Node MaterialAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(uuid);
        rootNode["Shader"] = static_cast<uint64_t>(shader);

        YAML::Node propertiesNode;
        for (const auto& [name, type, data, sampler, texture] : properties)
        {
            YAML::Node propertyNode;
            propertyNode["Name"] = name;
            propertyNode["Type"] = MaterialAssetPropertyTypeToString(type);
            propertyNode["Data"] = YAML::Binary(data, 64);
            propertyNode["Sampler"] = static_cast<uint64_t>(sampler);
            propertyNode["Texture"] = static_cast<uint64_t>(texture);
            propertiesNode.push_back(propertyNode);
        }
        rootNode["Properties"] = propertiesNode;

        return rootNode;
    }

    MaterialAssetMeta MaterialAssetMeta::Deserialize(const YAML::Node& node)
    {
        MaterialAssetMeta meta;

        meta.uuid = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();

        if (!node["Shader"])
        {
            meta.shader = UUID(-1);
            return meta;
        }

        meta.shader = UUID(node["Shader"].as<uint64_t>());

        if (node["Properties"])
        {
            for (const auto& propertyNode : node["Properties"])
            {
                MaterialAssetMetaProperty prop;

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

                meta.properties.push_back(prop);
            }
        }
        return meta;
    }

    void MaterialAssetMeta::UpdateForShader(AssetManager* assetManager)
    {
        if (shader == UUID(-1))
        {
            properties.clear();
            return;
        }

        ShaderAsset* shaderAsset = assetManager->GetAsset<ShaderAsset>(shader);
        shaderAsset->Load();

        auto reflection = shaderAsset->GetShader()->GetVertexShader()->GetReflection();
        for (auto& set : reflection.resourceGroups)
        {
            // only reflect material property
            if (set.set != 2)
            {
                continue;
            }
            for (auto& slot : set.slots)
            {
                if (slot.slot == 0)
                {
                    for (auto& member : slot.buffer.members)
                    {
                        bool propertyExists = false;
                        for (auto& metaProperty : properties)
                        {
                            if (metaProperty.name == member.name)
                            {
                                propertyExists = true;
                            }
                        }
                        if (!propertyExists)
                        {
                            MaterialAssetMetaProperty prop;
                            prop.name = member.name;
                            std::ranges::fill(prop.data, 0);
                            prop.sampler = UUID(-1);
                            prop.texture = UUID(-1);

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
                            properties.push_back(prop);
                        }
                    }
                }
                else
                {
                    switch (slot.type)
                    {
                        case RHIResourceBindingType::Sampler:
                        {
                            bool propertyExists = false;
                            for (auto& metaProperty : properties)
                            {
                                if (metaProperty.name == slot.variableName)
                                {
                                    propertyExists = true;
                                }
                            }
                            if (propertyExists)
                            {
                                break;
                            }
                            MaterialAssetMetaProperty prop;
                            prop.name = slot.variableName;
                            prop.type = MaterialAssetPropertyType::Sampler;
                            std::ranges::fill(prop.data, 0);
                            prop.sampler = UUID(-1);
                            prop.texture = UUID(-1);
                            properties.push_back(prop);
                            break;
                        }
                        case RHIResourceBindingType::SampledImage:
                        {
                            bool propertyExists = false;
                            for (auto& metaProperty : properties)
                            {
                                if (metaProperty.name == slot.variableName)
                                {
                                    propertyExists = true;
                                }
                            }
                            if (propertyExists)
                            {
                                break;
                            }
                            MaterialAssetMetaProperty prop;
                            prop.name = slot.variableName;
                            prop.type = MaterialAssetPropertyType::Texture;
                            std::ranges::fill(prop.data, 0);
                            prop.sampler = UUID(-1);
                            prop.texture = UUID(-1);
                            properties.push_back(prop);
                            break;
                        }
                        case RHIResourceBindingType::SamplerWithImage:
                        {
                            bool propertyExists = false;
                            for (auto& metaProperty : properties)
                            {
                                if (metaProperty.name == slot.variableName)
                                {
                                    propertyExists = true;
                                }
                            }
                            if (propertyExists)
                            {
                                break;
                            }
                            MaterialAssetMetaProperty prop;
                            prop.name = slot.variableName;
                            prop.type = MaterialAssetPropertyType::SamplerWithTexture;
                            std::ranges::fill(prop.data, 0);
                            prop.sampler = UUID(-1);
                            prop.texture = UUID(-1);
                            properties.push_back(prop);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
    }

    MaterialAsset::MaterialAsset(UUID uuid,
                                 AssetManager* assetManager,
                                 Renderer* renderer,
                                 std::filesystem::path filePath,
                                 MaterialAssetMeta meta)
        : m_UUID(uuid), m_AssetManager(assetManager), m_Renderer(renderer), m_FilePath(std::move(filePath)),
          m_Meta(std::move(meta)) {}

    MaterialAsset::MaterialAsset(MaterialAsset&& other) noexcept
        : m_UUID(other.m_UUID),
          m_IsLoaded(other.m_IsLoaded),
          m_AssetManager(other.m_AssetManager),
          m_Renderer(other.m_Renderer),
          m_FilePath(std::move(other.m_FilePath)),
          m_Meta(other.m_Meta)
    {
        other.m_Renderer = nullptr;
        other.m_IsLoaded = false;
    }

    MaterialAsset& MaterialAsset::operator=(MaterialAsset&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_UUID = other.m_UUID;
        m_IsLoaded = other.m_IsLoaded;
        m_AssetManager = other.m_AssetManager;
        m_FilePath = std::move(other.m_FilePath);
        m_Renderer = other.m_Renderer;
        m_Meta = other.m_Meta;

        other.m_Renderer = nullptr;
        other.m_IsLoaded = false;
        return *this;
    }

    MaterialAsset::~MaterialAsset()
    {
        Unload();
    }

    void MaterialAsset::Load()
    {
        if (m_IsLoaded)
        {
            return;
        }

        if (m_Meta.shader == UUID(-1))
        {
            m_MaterialID = 0;
            m_Material = nullptr;
            m_IsLoaded = true;
            return;
        }

        ShaderAsset* shaderAsset = m_AssetManager->GetAsset<ShaderAsset>(m_Meta.shader);
        shaderAsset->Load();

        // make sure the reflections of the two stages are the same!
        auto reflection = shaderAsset->GetShader()->GetVertexShader()->GetReflection();

        std::vector<MaterialProperty> properties;

        for (auto& metaProperty : m_Meta.properties)
        {
            for (auto& group : reflection.resourceGroups)
            {
                if (group.set != kMaterialPropertyResourceGroup) // 2
                {
                    continue;
                }

                // non-buffered properties
                for (auto& slot : group.slots)
                {
                    if (slot.variableName == metaProperty.name)
                    {
                        MaterialProperty prop;
                        prop.name = metaProperty.name;
                        prop.slot = slot.slot;
                        prop.isInBuffer = false;
                        switch (slot.type)
                        {
                            case RHIResourceBindingType::Sampler:
                            {
                                auto* sampler = m_AssetManager->GetAsset<SamplerAsset>(metaProperty.sampler);
                                if (sampler)
                                {
                                    sampler->Load();
                                    prop.sampler = sampler->GetSampler();
                                }
                                break;
                            }
                            case RHIResourceBindingType::SampledImage:
                            {
                                auto* image = m_AssetManager->GetAsset<TextureAsset>(metaProperty.texture);
                                if (image)
                                {
                                    image->Load();
                                    prop.texture = image->GetTexture();
                                }
                                break;
                            }
                            case RHIResourceBindingType::SamplerWithImage:
                            {
                                auto* image = m_AssetManager->GetAsset<TextureAsset>(metaProperty.texture);
                                auto* sampler = m_AssetManager->GetAsset<SamplerAsset>(metaProperty.sampler);
                                if (image)
                                {
                                    image->Load();
                                    prop.texture = image->GetTexture();
                                }
                                if (sampler)
                                {
                                    sampler->Load();
                                    prop.sampler = sampler->GetSampler();
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        properties.push_back(prop);
                    }
                }
                // buffered properties
                for (auto& slot : group.slots)
                {
                    if (slot.slot != 0)
                    {
                        continue;
                    }

                    for (auto& member : slot.buffer.members)
                    {
                        if (metaProperty.name == member.name)
                        {
                            MaterialProperty prop;
                            prop.name = metaProperty.name;
                            prop.slot = slot.slot;
                            prop.isInBuffer = true;
                            std::copy_n(metaProperty.data, 64, prop.data);
                            prop.member = member;
                            properties.push_back(prop);
                        }
                    }
                }
            }
        }

        auto material = std::make_unique<Material>(m_UUID, shaderAsset->GetShader(), properties);

        m_MaterialID = shaderAsset->GetShader()->RegisterMaterial(std::move(material));
        m_Material = shaderAsset->GetShader()->GetMaterial(m_MaterialID);

        m_IsLoaded = true;
    }

    void MaterialAsset::Unload()
    {
        if (!m_IsLoaded)
        {
            return;
        }

        if (m_Meta.shader == UUID(-1))
        {
            m_MaterialID = 0;
            m_Material = nullptr;
            m_IsLoaded = false;
            return;
        }

        ShaderAsset* shaderAsset = m_AssetManager->GetAsset<ShaderAsset>(m_Meta.shader);
        shaderAsset->GetShader()->UnregisterMaterial(m_MaterialID);

        m_MaterialID = 0;
        m_Material = nullptr;

        m_IsLoaded = false;
    }

    void MaterialAsset::Release()
    {
        Unload();
    }

    void MaterialAsset::Recreate()
    {
        if (m_IsLoaded)
        {
            m_Renderer->GetDevice()->WaitIdle();
            Unload();
            Load();
        }
    }
} // Hazel
