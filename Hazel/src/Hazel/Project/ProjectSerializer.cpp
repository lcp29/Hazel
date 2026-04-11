#include "ProjectSerializer.h"

#include "GlobalSettingRegistry.h"
#include "hzpch.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    ProjectSerializer::ProjectSerializer(Ref<Project> project)
        : m_Project(project)
    {
    }

    bool ProjectSerializer::Serialize(const std::filesystem::path& filepath)
    {
        const auto& config = m_Project->GetConfig();

        YAML::Emitter out;
        YAML::Node rootNode;
        YAML::Node projectNode;
        projectNode["Name"] = config.Name;
        projectNode["StartScene"] = config.StartScene.string();
        projectNode["AssetDirectory"] = config.AssetDirectory.string();
        projectNode["ScriptModulePath"] = config.ScriptModulePath.string();
        rootNode["Project"] = projectNode;
        rootNode["GlobalSettings"] = GlobalSettings.Serialize();

        out << rootNode;

        std::ofstream fout(filepath);
        fout << out.c_str();

        return true;
    }

    bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
    {
        auto& config = m_Project->GetConfig();

        YAML::Node data;
        try
        {
            data = YAML::LoadFile(filepath.string());
        }
        catch (const std::exception& e)
        {
            HZ_CORE_ERROR("Failed to load project file '{0}'\n     {1}", (const char*)filepath.c_str(), e.what());
            return false;
        }

        YAML::Node projectNode = data["Project"];
        if (!projectNode)
            return false;

        config.Name = projectNode["Name"] ? projectNode["Name"].as<std::string>() : "Untitled";
        config.StartScene = projectNode["StartScene"] ? projectNode["StartScene"].as<std::string>() : "";
        config.AssetDirectory =
            projectNode["AssetDirectory"] ? projectNode["AssetDirectory"].as<std::string>() : "Assets";
        config.ScriptModulePath =
            projectNode["ScriptModulePath"] ? projectNode["ScriptModulePath"].as<std::string>() : "";

        GlobalSettings.Deserialize(data["GlobalSettings"]);
        return true;
    }
} // namespace Hazel