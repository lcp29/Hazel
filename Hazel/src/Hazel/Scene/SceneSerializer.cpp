#include "SceneSerializer.h"

#include "Entity.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Project/Project.h"
// ======== Aster Modify Begin ========
#include "Hazel/Scene/Components.h"

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
        {
            const auto& viewportCameraTransform = m_Scene->GetViewportCameraController().GetCameraTransform();
            YAML::Node viewportCameraTransformNode;
            viewportCameraTransformNode["Translation"] = viewportCameraTransform.translation;
            viewportCameraTransformNode["Rotation"] = viewportCameraTransform.rotation;
            viewportCameraTransformNode["Scale"] = viewportCameraTransform.scale;
            rootNode["ViewportCameraTransform"] = viewportCameraTransformNode;
        }

        for (auto& enttEntity : m_Scene->m_Registry.view<entt::entity>())
        {
            Entity entity = {enttEntity, m_Scene.get()};
            if (!entity) continue;
            entities.push_back(entity.Serialize());
        }

        out << rootNode;
        // ======== Aster Modify End ========

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
        // ======== Aster Modify Begin ========
        catch (const YAML::ParserException& e)
        {
            HZ_CORE_ERROR("Failed to load .hazel file '{0}'\n     {1}", filepath, e.what());
            return false;
        }
        // ======== Aster Modify End ========

        if (!data["Scene"]) return false;

        // ======== Aster Modify Begin ========
        auto sceneName = data["Scene"].as<std::string>();
        HZ_CORE_TRACE("Deserializing scene '{0}'", sceneName);

        m_Scene->SetViewportCamera(Camera::Deserialize(data["ViewportCamera"]));
        if (auto viewportCameraTransformNode = data["ViewportCameraTransform"])
        {
            Aster::Transform viewportCameraTransform;
            viewportCameraTransform.translation = viewportCameraTransformNode["Translation"].as<glm::vec3>();
            viewportCameraTransform.rotation = viewportCameraTransformNode["Rotation"].as<glm::vec3>();
            viewportCameraTransform.scale = viewportCameraTransformNode["Scale"].as<glm::vec3>();
            m_Scene->GetViewportCameraController().SetCameraTransform(viewportCameraTransform);
        }

        if (auto entities = data["Entities"])
        {
            for (auto entityNode : entities)
            {
                Entity::DeserializeWithoutRelationship(entityNode, m_Scene.get());
            }

            for (auto entityNode : entities)
            {
                auto uuid = entityNode["ID"]["ID"][0].as<UUID>();
                Entity entity = m_Scene->GetEntityByUUID(uuid);
                entity.BuildRelationship(entityNode["Relationship"]);
                // ======== Aster Modify End ========
            }
        }

        return true;
    }
} // namespace Hazel
