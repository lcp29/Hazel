//
// Created by helmholtz on 2026/3/31.
//

#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/Shader.h"

#include <filesystem>
#include <optional>
#include <shaderc/shaderc.hpp>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Renderer;

    struct ShaderAssetMeta
    {
        UUID uuid = 0;

        YAML::Node Serialize() const;
        static ShaderAssetMeta Deserialize(const YAML::Node& node);
    };

    class ShaderAsset
    {
    public:
        ShaderAsset() = delete;

        ShaderAsset(Renderer* renderer,
                    std::filesystem::path filePath,
                    ShaderAssetMeta meta);

        ShaderAsset(const ShaderAsset&) = delete;
        ShaderAsset& operator=(const ShaderAsset&) = delete;
        ShaderAsset(ShaderAsset&& other) noexcept;
        ShaderAsset& operator=(ShaderAsset&& other) noexcept;
        ~ShaderAsset();

        void Load();
        void Unload();

        bool IsValid() const
        {
            return m_Shader && m_Shader->IsValid();
        }

        UUID GetUUID() const
        {
            return m_Meta.uuid;
        }

        const ShaderAssetMeta& GetMeta() const
        {
            return m_Meta;
        }

        const std::filesystem::path& GetFilePath() const
        {
            return m_FilePath;
        }

        Shader* GetShader()
        {
            return m_Shader;
        }

        const Shader* GetShader() const
        {
            return m_Shader;
        }

        bool IsLoaded() const
        {
            return m_IsLoaded;
        }

        void Release();

    private:
        bool m_IsLoaded = false;
        UUID m_UUID = 0;
        ShaderAssetMeta m_Meta{};
        std::filesystem::path m_FilePath;
        shaderc::SpvCompilationResult m_VertexCompileResult;
        shaderc::SpvCompilationResult m_FragmentCompileResult;
        Renderer* m_Renderer = nullptr;
        Shader* m_Shader = nullptr;
    };
} // namespace Hazel
