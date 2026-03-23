#include "hzpch.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Project/Project.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    void SceneSerializer::Serialize(const std::string& filepath) const
    {
        YAML::Emitter out;

        YAML::Node rootNode, entities(YAML::NodeType::Sequence);
        rootNode["Scene"] = m_Scene->GetName();
        rootNode["Entities"] = entities;
        rootNode["ViewportCamera"] = m_Scene->GetViewportCamera().Serialize();

        for (auto& enttEntity : m_Scene->m_Registry.view<entt::entity>())
        {
            Entity entity = {enttEntity, m_Scene.get()};
            if (!entity)
                continue;
            entities.push_back(entity.Serialize());
        }

        out << rootNode;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    bool SceneSerializer::Deserialize(const std::string& filepath)
    {
        YAML::Node data;
        try
        {
            data = YAML::LoadFile(filepath);
        }
        catch (YAML::ParserException e)
        {
            HZ_CORE_ERROR("Failed to load .hazel file '{0}'\n     {1}", filepath, e.what());
            return false;
        }

        if (!data["Scene"])
            return false;

        std::string sceneName = data["Scene"].as<std::string>();
        HZ_CORE_TRACE("Deserializing scene '{0}'", sceneName);

        m_Scene->SetViewportCamera(Camera::Deserialize(data["ViewportCamera"]));

        if (auto entities = data["Entities"])
        {
            for (auto entityNode : entities)
            {
                Entity::DeserializeWithoutRelationship(entityNode, m_Scene.get());
            }

            for (auto entityNode : entities)
            {
                UUID uuid = entityNode["ID"]["ID"].as<UUID>();
                Entity entity = m_Scene->GetEntityByUUID(uuid);
                entity.BuildRelationship(entityNode["Relationship"]);
            }
        }

        return true;
    }
}
