#pragma once

// ======== Aster Modify Begin ========
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Core/Base.h"

#include <filesystem>
#include <memory>
#include <string>

// ======== Aster Modify End ========

namespace Hazel
{
    // ======== Aster Modify Begin ========
    class Renderer;

    // ======== Aster Modify End ========

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
        // ======== Aster Modify Begin ========
        static bool HasActive() { return static_cast<bool>(s_ActiveProject); }

        static const std::filesystem::path& GetProjectDirectory()
        // ======== Aster Modify End ========
        {
            HZ_CORE_ASSERT(s_ActiveProject);
            return s_ActiveProject->m_ProjectDirectory;
        }

        // ======== Aster Modify Begin ========
        static const std::filesystem::path& GetProjectFilePath()
        {
            HZ_CORE_ASSERT(s_ActiveProject);
            return s_ActiveProject->m_ProjectFilePath;
        }

        static std::filesystem::path GetAssetDirectory()
        // ======== Aster Modify End ========
        {
            HZ_CORE_ASSERT(s_ActiveProject);
            return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
        }

        static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
        {
            HZ_CORE_ASSERT(s_ActiveProject);
            return GetAssetDirectory() / path;
        }

        ProjectConfig& GetConfig() { return m_Config; }

        // ======== Aster Modify Begin ========
        Aster::AssetManager* GetAssetManager() { return m_AssetManager.get(); }

        const Aster::AssetManager* GetAssetManager() const { return m_AssetManager.get(); }

        // ======== Aster Modify End ========

        static Ref<Project> GetActive() { return s_ActiveProject; }

        static Ref<Project> New();
        // ======== Aster Modify Begin ========
        static Ref<Project> Load(const std::filesystem::path& path, Renderer* renderer);
        static bool SaveActive(const std::filesystem::path& path);

        static void CloseActive() { s_ActiveProject.reset(); }

        // ======== Aster Modify End ========

      private:
        ProjectConfig m_Config;
        std::filesystem::path m_ProjectDirectory;
        // ======== Aster Modify Begin ========
        std::filesystem::path m_ProjectFilePath;
        std::unique_ptr<Aster::AssetManager> m_AssetManager;
        // ======== Aster Modify End ========

        inline static Ref<Project> s_ActiveProject;
    };
} // namespace Hazel
