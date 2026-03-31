#pragma once

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Core/Base.h"

#include <filesystem>
#include <string>

namespace Hazel
{
    class Renderer;

    struct ProjectConfig
    {
        std::string Name = "Untitled";
        std::filesystem::path StartScene;
        std::filesystem::path AssetDirectory;
        std::filesystem::path ScriptModulePath;
    };

    class Project
    {
    public:
        static bool HasActive()
        {
            return static_cast<bool>(s_ActiveProject);
        }

        static const std::filesystem::path& GetProjectDirectory()
        {
            HZ_CORE_ASSERT(s_ActiveProject);
            return s_ActiveProject->m_ProjectDirectory;
        }

        static const std::filesystem::path& GetProjectFilePath()
        {
            HZ_CORE_ASSERT(s_ActiveProject);
            return s_ActiveProject->m_ProjectFilePath;
        }

        static std::filesystem::path GetAssetDirectory()
        {
            HZ_CORE_ASSERT(s_ActiveProject);
            return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
        }

        // TODO(Yan): move to asset manager when we have one
        static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
        {
            HZ_CORE_ASSERT(s_ActiveProject);
            return GetAssetDirectory() / path;
        }

        ProjectConfig& GetConfig()
        {
            return m_Config;
        }

        static Ref<Project> GetActive()
        {
            return s_ActiveProject;
        }

        static Ref<Project> New();
        static Ref<Project> Load(const std::filesystem::path& path, Renderer* renderer);
        static bool SaveActive(const std::filesystem::path& path);

        static void CloseActive()
        {
            s_ActiveProject.reset();
        }

    private:
        ProjectConfig m_Config;
        std::filesystem::path m_ProjectDirectory;
        std::filesystem::path m_ProjectFilePath;
        AssetManager m_AssetManager;

        inline static Ref<Project> s_ActiveProject;
    };
} // namespace Hazel
