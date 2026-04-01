//
// Created by helmholtz on 2026/4/1.
//

#pragma once
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/Material.h"

#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Renderer;
    class AssetManager;
    class ShaderAsset;

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

    struct MaterialAssetMetaProperty
    {
        std::string name;
        MaterialAssetPropertyType type;
        uint8_t data[64];
        UUID sampler;
        UUID texture;
    };

    struct MaterialAssetMeta
    {
        UUID uuid;
        UUID shader;
        std::vector<MaterialAssetMetaProperty> properties;

        YAML::Node Serialize() const;
        static MaterialAssetMeta Deserialize(const YAML::Node& node);

        void UpdateForShader(AssetManager* assetManager);
    };

    class MaterialAsset
    {
    public:
        MaterialAsset() = delete;

        MaterialAsset(UUID uuid,
                      AssetManager* assetManager,
                      Renderer* renderer,
                      std::filesystem::path filePath,
                      MaterialAssetMeta meta);

        MaterialAsset(const MaterialAsset&) = delete;
        MaterialAsset& operator=(const MaterialAsset&) = delete;
        MaterialAsset(MaterialAsset&& other) noexcept;
        MaterialAsset& operator=(MaterialAsset&& other) noexcept;
        ~MaterialAsset();

        void Load();
        void Unload();

        void Release();
        void Recreate();

        void SetMaterialID(uint32_t id)
        {
            m_MaterialID = id;
        }

        uint32_t GetMaterialID() const
        {
            return m_MaterialID;
        }

        const MaterialAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        MaterialAssetMeta& GetMeta()
        {
            return m_Meta;
        }

        Material* GetMaterial() const
        {
            return m_Material;
        }

        std::filesystem::path GetFilePath() const
        {
            return m_FilePath;
        }

        bool IsLoaded() const
        {
            return m_IsLoaded;
        }

    private:
        UUID m_UUID = 0;
        bool m_IsLoaded = false;
        uint32_t m_MaterialID = 0;
        AssetManager* m_AssetManager = nullptr;
        Renderer* m_Renderer = nullptr;
        Material* m_Material = nullptr;
        std::filesystem::path m_FilePath;
        MaterialAssetMeta m_Meta{};
    };
} // Hazel