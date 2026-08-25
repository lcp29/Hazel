#pragma once
#include "Hazel/Core/UUID.h"
#include "Scene.h"

namespace Hazel
{
    // ======== Aster Modify Begin ========
    struct MeshRendererComponent;
    class Scene;

    class Entity
    // ======== Aster Modify End ========
    {
      public:
        Entity() = default;

        Entity(entt::entity handle, Scene* scene)
            // ======== Aster Modify Begin ========
            : m_EntityHandle(handle)
            , m_Scene(scene)
        {}

        // ======== Aster Modify End ========

        Entity(const Entity& other) = default;

        template <typename T, typename... Args> T& AddComponent(Args&&... args)
        {
            HZ_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
            T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
            m_Scene->OnComponentAdded<T>(*this, component);
            // ======== Aster Modify Begin ========
            if constexpr (std::is_same_v<T, MeshRendererComponent>) { AddMeshRendererPayload(component); }
            // ======== Aster Modify End ========
            return component;
        }

        template <typename T, typename... Args> T& AddOrReplaceComponent(Args&&... args)
        {
            T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
            m_Scene->OnComponentAdded<T>(*this, component);
            return component;
        }

        template <typename T> T& GetComponent()
        {
            HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            return m_Scene->m_Registry.get<T>(m_EntityHandle);
        }

        // ======== Aster Modify Begin ========
        template <typename T> bool HasComponent() { return m_Scene->m_Registry.any_of<T>(m_EntityHandle); }

        // ======== Aster Modify End ========

        template <typename T> void RemoveComponent()
        {
            HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            // ======== Aster Modify Begin ========
            if constexpr (std::is_same_v<T, MeshRendererComponent>) { RemoveMeshRendererPayload(); }
            // ======== Aster Modify End ========
            m_Scene->m_Registry.remove<T>(m_EntityHandle);
        }

        // ======== Aster Modify Begin ========
        void AddChild(Entity* child);
        void RemoveChild(Entity* child);
        void Detach();
        // ======== Aster Modify End ========

        operator bool() const;
        operator entt::entity() const;
        operator uint32_t() const;

        UUID GetUUID();
        const std::string& GetName();

        bool operator==(const Entity& other) const;
        // ======== Aster Modify Begin ========
        bool operator!=(const Entity& other) const;

        YAML::Node Serialize();
        static Entity DeserializeWithoutRelationship(const YAML::Node& node, Scene* scene);
        void BuildRelationship(const YAML::Node& node);

        void SetTransform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale);
        void SetTransform(const glm::mat4& transform);
        void SetTranslation(const glm::vec3& translation);
        void SetRotation(const glm::vec3& rotation);
        void SetScale(const glm::vec3& scale);
        void SetMesh(UUID mesh);
        void SetMaterial(UUID material);

        Scene* GetScene() const { return m_Scene; }

      private:
        glm::mat4 GetGlobalTransform() const;
        void AddMeshRendererPayload(const MeshRendererComponent& component);
        void RemoveMeshRendererPayload();
        // ======== Aster Modify End ========

        entt::entity m_EntityHandle{entt::null};
        Scene* m_Scene = nullptr;
    };
} // namespace Hazel