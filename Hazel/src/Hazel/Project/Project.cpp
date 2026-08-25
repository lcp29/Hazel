#include "Project.h"

#include "ProjectSerializer.h"
// ======== Aster Modify Begin ========
#include "hzpch.h"

#include <complex>

// ======== Aster Modify End ========

namespace Hazel
{
    Ref<Project> Project::New()
    {
        s_ActiveProject = CreateRef<Project>();
        // ======== Aster Modify Begin ========
        s_ActiveProject->m_ProjectDirectory.clear();
        s_ActiveProject->m_ProjectFilePath.clear();
        // ======== Aster Modify End ========
        return s_ActiveProject;
    }

    // ======== Aster Modify Begin ========
    Ref<Project> Project::Load(const std::filesystem::path& path, Renderer* renderer)
    // ======== Aster Modify End ========
    {
        Ref<Project> project = CreateRef<Project>();

        ProjectSerializer serializer(project);
        if (serializer.Deserialize(path))
        {
            project->m_ProjectDirectory = path.parent_path();
            // ======== Aster Modify Begin ========
            project->m_ProjectFilePath = path;
            s_ActiveProject = project;
            project->m_AssetManager = std::make_unique<Aster::AssetManager>(project.get(), renderer);
            project->m_AssetManager->InitializeAssetRegistry();
            // ======== Aster Modify End ========
            return s_ActiveProject;
        }

        return nullptr;
    }

    bool Project::SaveActive(const std::filesystem::path& path)
    {
        ProjectSerializer serializer(s_ActiveProject);
        if (serializer.Serialize(path))
        {
            s_ActiveProject->m_ProjectDirectory = path.parent_path();
            // ======== Aster Modify Begin ========
            s_ActiveProject->m_ProjectFilePath = path;
            s_ActiveProject->m_AssetManager->WriteAllMetaFiles();
            // ======== Aster Modify End ========
            return true;
        }

        return false;
    }
} // namespace Hazel
