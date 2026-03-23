//
// Created by helmholtz on 2026/3/20.
//

#include "Entity.h"

namespace Hazel
{
    void Entity::AddChild(Entity* child)
    {
        HZ_CORE_ASSERT(child->m_Scene == m_Scene, "Child entity must belong to the same scene!");
        auto& childRelationship = GetComponent<EntityRelationshipComponent>();
        childRelationship.childCount++;
        if (childRelationship.firstChild == entt::null)
        {
            childRelationship.firstChild = child->m_EntityHandle;
        }
        else
        {
            auto sibling = Entity{childRelationship.firstChild, m_Scene};
            for (int i = 0; i < childRelationship.childCount - 2; i++)
            {
                sibling = Entity{sibling.GetComponent<EntityRelationshipComponent>().nextSibling, m_Scene};
            }
            sibling.GetComponent<EntityRelationshipComponent>().nextSibling = child->m_EntityHandle;
            child->GetComponent<EntityRelationshipComponent>().prevSibling = sibling.m_EntityHandle;
        }
    }

    void Entity::RemoveChild(Entity* child)
    {
        HZ_CORE_ASSERT(child->m_Scene == m_Scene, "Child entity must belong to the same scene!");
        auto& childRelationship = GetComponent<EntityRelationshipComponent>();
        if (childRelationship.childCount == 0)
            return;
        childRelationship.childCount--;
        if (childRelationship.firstChild == child->m_EntityHandle)
        {
            childRelationship.firstChild = child->GetComponent<EntityRelationshipComponent>().nextSibling;
            if (childRelationship.firstChild != entt::null)
            {
                Entity{childRelationship.firstChild, m_Scene}.GetComponent<EntityRelationshipComponent>().prevSibling =
                    entt::null;
            }
        }
        else
        {
            auto sibling = Entity{childRelationship.firstChild, m_Scene};
            while (sibling.m_EntityHandle != entt::null)
            {
                if (sibling.GetComponent<EntityRelationshipComponent>().nextSibling == child->m_EntityHandle)
                {
                    sibling.GetComponent<EntityRelationshipComponent>().nextSibling =
                        child->GetComponent<EntityRelationshipComponent>().nextSibling;
                    if (sibling.GetComponent<EntityRelationshipComponent>().nextSibling != entt::null)
                    {
                        Entity{sibling.GetComponent<EntityRelationshipComponent>().nextSibling, m_Scene}
                            .GetComponent<EntityRelationshipComponent>().prevSibling = sibling.m_EntityHandle;
                    }
                    break;
                }
                sibling = Entity{sibling.GetComponent<EntityRelationshipComponent>().nextSibling, m_Scene};
            }
        }
    }

    void Entity::Detach()
    {
        auto& relationship = GetComponent<EntityRelationshipComponent>();
        if (relationship.parent != entt::null)
        {
            Entity parent{relationship.parent, m_Scene};
            parent.RemoveChild(this);
        }
    }

    Entity::operator bool() const
    {
        return m_EntityHandle != entt::null;
    }

    Entity::operator entt::entity() const
    {
        return m_EntityHandle;
    }

    Entity::operator uint32_t() const
    {
        return static_cast<uint32_t>(m_EntityHandle);
    }

    UUID Entity::GetUUID()
    {
        return GetComponent<IDComponent>().ID;
    }

    const std::string& Entity::GetName()
    {
        return GetComponent<TagComponent>().tag;
    }

    bool Entity::operator==(const Entity& other) const
    {
        return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
    }

    bool Entity::operator!=(const Entity& other) const
    {
        return !(*this == other);
    }

    YAML::Node Entity::Serialize()
    {
        YAML::Node node;
        node["ID"] = GetComponent<IDComponent>().Serialize();
        node["Tag"] = GetComponent<TagComponent>().Serialize();
        node["Transform"] = GetComponent<TransformComponent>().Serialize();
        node["Relationship"] = GetComponent<EntityRelationshipComponent>().Serialize(m_Scene->m_Registry);
        if (HasComponent<CameraComponent>())
        {
            node["Camera"] = GetComponent<CameraComponent>().Serialize();
        }
        if (HasComponent<MeshRendererComponent>())
        {
            node["MeshRenderer"] = GetComponent<MeshRendererComponent>().Serialize();
        }
        if (HasComponent<ScriptComponent>())
        {
            node["Script"] = GetComponent<ScriptComponent>().Serialize(*this);
        }
        return node;
    }

    Entity Entity::DeserializeWithoutRelationship(const YAML::Node& node, Scene* scene)
    {
        IDComponent idComponent = IDComponent::Deserialize(node["ID"]);
        TagComponent tagComponent = TagComponent::Deserialize(node["Tag"]);
        TransformComponent transformComponent = TransformComponent::Deserialize(node["Transform"]);
        Entity deserializedEntity = scene->CreateEntityWithUUID(idComponent.ID, tagComponent.tag);
        deserializedEntity.AddOrReplaceComponent<TransformComponent>(transformComponent);
        if (node["Camera"])
        {
            deserializedEntity.AddComponent<CameraComponent>(CameraComponent::Deserialize(node["Camera"]));
        }
        if (node["MeshRenderer"])
        {
            deserializedEntity.AddComponent<MeshRendererComponent>(
                MeshRendererComponent::Deserialize(node["MeshRenderer"]));
        }
        if (node["Script"])
        {
            deserializedEntity.AddComponent<ScriptComponent>(
                ScriptComponent::Deserialize(node["Script"], deserializedEntity));
        }
        return deserializedEntity;
    }

    void Entity::BuildRelationship(const YAML::Node& node)
    {
        EntityRelationshipComponent relationshipComponent = EntityRelationshipComponent::Deserialize(
            node["Relationship"], m_Scene->GetMapUUIDToEntity());
        AddOrReplaceComponent<EntityRelationshipComponent>(relationshipComponent);
    }
}
