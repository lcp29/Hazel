//
// Created by helmholtz on 2026/4/2.
//

#pragma once
#include "Hazel/Core/UUID.h"

#include <condition_variable>
#include <filesystem>
#include <mutex>

namespace Hazel
{
    class GPUAsset;

    enum class AssetType
    {
        Unknown,
        Texture,
        Shader,
        Sampler,
        RenderTexture,
        ComputeShader,
        Mesh,
        Material,
        GraphicsPipeline
    };

    enum class AssetState
    {
        Unloaded,
        Loading,
        Loaded,
        Failed
    };

    struct AssetRegistryTerm
    {
        UUID uuid;
        AssetType type;
        AssetState state = AssetState::Unloaded;
        std::mutex mutex;
        std::condition_variable loadingCondition;
        std::filesystem::path filePath;

        AssetRegistryTerm() = default;

        AssetRegistryTerm(UUID uuid, AssetType type, std::filesystem::path filePath)
            : uuid(uuid), type(type), filePath(std::move(filePath)) {}
    };

    enum class GPUAssetLoadState
    {
        Unloaded,
        Loading,
        Loaded
    };

    struct GPUAssetState
    {
        std::mutex mutex;
        std::condition_variable condition;
        GPUAssetLoadState state = GPUAssetLoadState::Unloaded;
        uint64_t resolvedVersion = 0;
    };

    class Asset
    {
    public:
        Asset() = delete;

        explicit Asset(AssetRegistryTerm* registryTerm)
            : m_RegistryTerm(registryTerm) {}

        virtual ~Asset() = default;

        UUID GetUUID() const
        {
            return m_RegistryTerm->uuid;
        }

        const std::filesystem::path& GetFilePath() const
        {
            return m_RegistryTerm->filePath;
        }

        virtual uint64_t GetVersion() const = 0;
        virtual void VersionUp() = 0;

        AssetType GetType() const
        {
            return m_RegistryTerm->type;
        }

        GPUAssetState& GetGPUAssetState()
        {
            return m_GPUAssetState;
        }

        std::filesystem::path GetAbsoluteFilePath() const
        {
            return std::filesystem::absolute(m_RegistryTerm->filePath);
        }

    protected:
        AssetRegistryTerm* m_RegistryTerm = nullptr;
        GPUAssetState m_GPUAssetState{};
    };
} // Hazel