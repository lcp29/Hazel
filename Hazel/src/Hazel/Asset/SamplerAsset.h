//
// Created by helmholtz on 2026/3/25.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/Sampler.h"

#include <filesystem>
#include <optional>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Renderer;

    struct SamplerAssetMeta
    {
        UUID uuid = 0;
        RHISamplerDesc desc;

        YAML::Node Serialize() const;
        static SamplerAssetMeta Deserialize(const YAML::Node& node);
    };

    class SamplerAsset
    {
    public:
        SamplerAsset() = default;

        SamplerAsset(UUID uuid,
                     std::filesystem::path filePath,
                     Renderer* renderer,
                     SamplerAssetMeta meta)
            : m_UUID(uuid)
              , m_FilePath(std::move(filePath))
              , m_Renderer(renderer)
              , m_Meta(meta) {};

        SamplerAsset(const SamplerAsset&) = delete;
        SamplerAsset& operator=(const SamplerAsset&) = delete;
        SamplerAsset(SamplerAsset&& other) noexcept;
        SamplerAsset& operator=(SamplerAsset&& other) noexcept;
        ~SamplerAsset();

        void Load();
        void Unload();

        UUID GetUUID() const
        {
            return m_UUID;
        }

        const std::filesystem::path& GetFilePath() const
        {
            return m_FilePath;
        }

        const RHISamplerDesc& GetDesc() const
        {
            return m_Meta.desc;
        }

        Sampler* GetSampler() const
        {
            return m_Sampler;
        }

        bool IsLoaded() const
        {
            return m_IsLoaded;
        }

        void Release();

    private:
        UUID m_UUID = 0;
        bool m_IsLoaded = false;
        std::filesystem::path m_FilePath;
        Renderer* m_Renderer = nullptr;
        SamplerAssetMeta m_Meta{};
        Sampler* m_Sampler = nullptr;
    };
} // namespace Hazel