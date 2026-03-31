//
// Created by helmholtz on 2026/3/29.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/ComputeShader.h"

#include <filesystem>
#include <optional>
#include <shaderc/shaderc.hpp>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Renderer;

    struct ComputeShaderAssetMeta
    {
        UUID uuid = 0;

        YAML::Node Serialize() const;
        static ComputeShaderAssetMeta Deserialize(const YAML::Node& node);
    };

    class ComputeShaderAsset
    {
    public:
        ComputeShaderAsset() = delete;

        ComputeShaderAsset(Renderer* renderer,
                           std::filesystem::path filePath,
                           ComputeShaderAssetMeta meta);

        ComputeShaderAsset(const ComputeShaderAsset&) = delete;
        ComputeShaderAsset& operator=(const ComputeShaderAsset&) = delete;
        ComputeShaderAsset(ComputeShaderAsset&& other) noexcept;
        ComputeShaderAsset& operator=(ComputeShaderAsset&& other) noexcept;
        ~ComputeShaderAsset();

        void Load();
        void Unload();

        bool IsValid() const
        {
            return m_ComputeShader && m_ComputeShader->IsValid();
        }

        UUID GetUUID() const
        {
            return m_Meta.uuid;
        }

        const ComputeShaderAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        const std::filesystem::path& GetFilePath() const
        {
            return m_FilePath;
        }

        ComputeShader* GetComputeShader()
        {
            return m_ComputeShader;
        }

        const ComputeShader* GetComputeShader() const
        {
            return m_ComputeShader;
        }

        bool IsLoaded() const
        {
            return m_IsLoaded;
        }

        void Release();

    private:
        bool m_IsLoaded = false;
        UUID m_UUID = 0;
        ComputeShaderAssetMeta m_Meta{};
        std::filesystem::path m_FilePath;
        shaderc::SpvCompilationResult m_CompileResult;
        Renderer* m_Renderer = nullptr;
        ComputeShader* m_ComputeShader = nullptr;
    };
} // namespace Hazel