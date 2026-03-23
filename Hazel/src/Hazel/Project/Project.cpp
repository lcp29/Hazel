#include "Project.h"

#include "ProjectSerializer.h"
#include "hzpch.h"

namespace Hazel
{
    Ref<Project> Project::New()
    {
        s_ActiveProject = CreateRef<Project>();
        s_ActiveProject->m_ProjectDirectory.clear();
        s_ActiveProject->m_ProjectFilePath.clear();
        return s_ActiveProject;
    }

    Ref<Project> Project::Load(const std::filesystem::path& path)
    {
        Ref<Project> project = CreateRef<Project>();

        ProjectSerializer serializer(project);
        if (serializer.Deserialize(path))
        {
            project->m_ProjectDirectory = path.parent_path();
            project->m_ProjectFilePath = path;
            s_ActiveProject = project;
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
            s_ActiveProject->m_ProjectFilePath = path;
            return true;
        }

        return false;
    }
} // namespace Hazel