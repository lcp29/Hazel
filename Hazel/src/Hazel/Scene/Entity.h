#pragma once

#include "Hazel/Core/UUID.h"
#include "Scene.h"

namespace Hazel
{
    class Entity
    {
    public:
        Entity() = default;

        Entity(entt::entity handle, Scene* scene)
            : m_EntityHandle(handle)
              , m_Scene(scene) {}

        Entity(const Entity& other) = default;

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            HZ_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
            T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
            m_Scene->OnComponentAdded<T>(*this, component);
            return component;
        }

        template <typename T, typename... Args>
        T& AddOrReplaceComponent(Args&&... args)
        {
            T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
            m_Scene->OnComponentAdded<T>(*this, component);
            return component;
        }

        template <typename T>
        T& GetComponent()
        {
            HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            return m_Scene->m_Registry.get<T>(m_EntityHandle);
        }

        template <typename T>
        bool HasComponent()
        {
            return m_Scene->m_Registry.any_of<T>(m_EntityHandle);
        }

        template <typename T>
        void RemoveComponent()
        {
            HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            m_Scene->m_Registry.remove<T>(m_EntityHandle);
        }

        void AddChild(Entity* child);
        void RemoveChild(Entity* child);
        void Detach();

        operator bool() const;
        operator entt::entity() const;
        operator uint32_t() const;

        UUID GetUUID();
        const std::string& GetName();

        bool operator==(const Entity& other) const;
        bool operator!=(const Entity& other) const;

        YAML::Node Serialize();
        static Entity DeserializeWithoutRelationship(const YAML::Node& node, Scene* scene);
        void BuildRelationship(const YAML::Node& node);

    private:
        entt::entity m_EntityHandle{entt::null};
        Scene* m_Scene = nullptr;
    };
} // namespace Hazel