#include "Scene.h"

#include "Components.h"
#include "Entity.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "ScriptableEntity.h"
#include "hzpch.h"

#include <glm/glm.hpp>

namespace Hazel
{
    Scene::Scene()
    {
        m_ViewportCamera = Camera::Perspective(45.0f, 1.778f, 0.01f, 1000.0f);
    }

    Scene::~Scene() {}

    template <typename... Component>
    static void
    CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        (
            [&]()
            {
                auto view = src.view < Component > ();
                for (auto srcEntity : view)
                {
                    entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

                    auto& srcComponent = src.get<Component>(srcEntity);
                    dst.emplace_or_replace<Component>(dstEntity, srcComponent);
                }
            }(),
            ...);
    }

    template <typename... Component>
    static void CopyComponent(ComponentGroup<Component...>,
                              entt::registry& dst,
                              entt::registry& src,
                              const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        CopyComponent<Component...>(dst, src, enttMap);
    }

    template <typename... Component>
    static void CopyComponentIfExists(Entity dst, Entity src)
    {
        (
            [&]()
            {
                if (src.HasComponent<Component>())
                    dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
            }(),
            ...);
    }

    template <typename... Component>
    static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
    {
        CopyComponentIfExists<Component...>(dst, src);
    }

    Ref<Scene> Scene::Copy(Ref<Scene> other)
    {
        Ref < Scene > newScene = CreateRef<Scene>();

        newScene->m_ViewportWidth = other->m_ViewportWidth;
        newScene->m_ViewportHeight = other->m_ViewportHeight;

        auto& srcSceneRegistry = other->m_Registry;
        auto& dstSceneRegistry = newScene->m_Registry;
        std::unordered_map<UUID, entt::entity> enttMap;

        // Create entities in new scene
        auto idView = srcSceneRegistry.view<IDComponent>();
        for (auto e : idView)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
            const auto& name = srcSceneRegistry.get<TagComponent>(e).tag;
            Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
            enttMap[uuid] = (entt::entity)newEntity;
        }

        // Copy components (except IDComponent and TagComponent)
        CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

        return newScene;
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityWithUUID(UUID(), name);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
    {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<TransformComponent>();
        auto& tag = entity.AddComponent<TagComponent>();
        tag.tag = name.empty() ? "Entity" : name;
        entity.AddComponent<EntityRelationshipComponent>();

        m_EntityMap[uuid] = entity;

        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_EntityMap.erase(entity.GetUUID());
        m_Registry.destroy(entity);
    }

    void Scene::OnRuntimeStart()
    {
        m_IsRunning = true;

        // Scripting
        {
            ScriptEngine::OnRuntimeStart(this);
            // Instantiate all script entities

            auto view = m_Registry.view < ScriptComponent > ();
            for (auto e : view)
            {
                Entity entity = {e, this};
                ScriptEngine::OnCreateEntity(entity);
            }
        }
    }

    void Scene::OnRuntimeStop()
    {
        m_IsRunning = false;

        ScriptEngine::OnRuntimeStop();
    }

    void Scene::OnUpdateRuntime(Timestep ts)
    {
        if (!m_IsPaused || m_StepFrames-- > 0)
        {
            // Update scripts
            {
                // C# Entity OnUpdate
                auto view = m_Registry.view < ScriptComponent > ();
                for (auto e : view)
                {
                    Entity entity = {e, this};
                    ScriptEngine::OnUpdateEntity(entity, ts);
                }

                auto scriptView = m_Registry.view < NativeScriptComponent > ();

                for (auto [entity, nsc] : scriptView.each())
                {
                    if (!nsc.instance)
                    {
                        nsc.instance = nsc.instantiateScript();
                        nsc.instance->m_Entity = Entity{entity, this};
                        nsc.instance->OnCreate();
                    }

                    nsc.instance->OnUpdate(ts);
                }
            }
        }
    }

    void Scene::OnUpdateSimulation(Timestep ts)
    {
        if (!m_IsPaused || m_StepFrames-- > 0)
        {
            // do something
        }
    }

    void Scene::OnUpdateEditor(Timestep ts) {}

    void Scene::OnViewportResize(uint32_t width, uint32_t height)
    {
        if (m_ViewportWidth == width && m_ViewportHeight == height)
            return;

        m_ViewportWidth = width;
        m_ViewportHeight = height;

        m_ViewportCamera.SetViewportSizeKeepFovY(width, height);
    }

    void Scene::Step(int frames)
    {
        m_StepFrames = frames;
    }

    Entity Scene::DuplicateEntity(Entity entity)
    {
        // Copy name because we're going to modify component data structure
        std::string name = entity.GetName();
        Entity newEntity = CreateEntity(name);
        CopyComponentIfExists(AllComponents{}, newEntity, entity);
        return newEntity;
    }

    Entity Scene::FindEntityByName(std::string_view name)
    {
        auto view = m_Registry.view < TagComponent > ();
        for (auto entity : view)
        {
            const TagComponent& tc = view.get<TagComponent>(entity);
            if (tc.tag == name)
                return Entity{entity, this};
        }
        return {};
    }

    Entity Scene::GetEntityByUUID(UUID uuid)
    {
        // TODO(Yan): Maybe should be assert
        if (m_EntityMap.find(uuid) != m_EntityMap.end())
            return {m_EntityMap.at(uuid), this};

        return {};
    }

    template <typename T>
    void Scene::OnComponentAdded(Entity entity, T& component)
    {
        static_assert(sizeof(T) == 0);
    }

    template <>
    void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component) {}

    template <>
    void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component) {}

    template <>
    void Scene::OnComponentAdded<EntityRelationshipComponent>(Entity entity, EntityRelationshipComponent& component) {}

    template <>
    void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component) {}

    template <>
    void Scene::OnComponentAdded<MeshRendererComponent>(Entity entity, MeshRendererComponent& component) {}

    template <>
    void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component) {}

    template <>
    void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component) {}

    template <>
    void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component) {}
} // namespace Hazel