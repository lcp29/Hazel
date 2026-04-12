#include "Scene.h"

#include "Components.h"
#include "Entity.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "ScriptableEntity.h"
#include "hzpch.h"

#include <glm/glm.hpp>

namespace Hazel
{
    namespace
    {
        glm::mat4 BuildTransformMatrix(const TransformComponent& transform)
        {
            const glm::mat4 rotationMatrix = glm::toMat4(glm::quat(transform.rotation));
            return glm::translate(glm::mat4(1.0f), transform.translation) * rotationMatrix
                   * glm::scale(glm::mat4(1.0f), transform.scale);
        }
    } // namespace

    Scene::Scene()
    {
        m_ViewportCamera = Camera::Perspective(45.0f, 1.778f, 0.01f, 1000.0f);
        m_ViewportCameraController.SetCamera(&m_ViewportCamera);
    }

    Scene::~Scene() {}

    template <typename... Component>
    static void
    CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        (
            [&]() {
                auto view = src.view<Component>();
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

    template <typename... Component> static void CopyComponentIfExists(Entity dst, Entity src)
    {
        (
            [&]() {
                if (src.HasComponent<Component>()) dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
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
        Ref<Scene> newScene = CreateRef<Scene>();

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
            enttMap[uuid] = static_cast<entt::entity>(newEntity);
        }

        // Copy components (except IDComponent and TagComponent)
        CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

        return newScene;
    }

    Entity Scene::CreateEntity(const std::string& name) { return CreateEntityWithUUID(UUID(), name); }

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
        if (entity.HasComponent<MeshRendererComponent>())
        {
            RenderSceneUpdatePayload payload{};
            payload.entity = entity.GetUUID();
            payload.type = RenderSceneUpdatePayload::Type::Remove;
            AddToRenderSceneUpdatePayload(payload);
        }

        auto relationship = entity.GetComponent<EntityRelationshipComponent>();
        if (relationship.prevSibling != entt::null)
        {
            auto& prevSiblingRelationship = m_Registry.get<EntityRelationshipComponent>(relationship.prevSibling);
            prevSiblingRelationship.nextSibling = relationship.nextSibling;
        }
        if (relationship.nextSibling != entt::null)
        {
            auto& nextSiblingRelationship = m_Registry.get<EntityRelationshipComponent>(relationship.nextSibling);
            nextSiblingRelationship.prevSibling = relationship.prevSibling;
        }
        if (relationship.parent != entt::null)
        {
            auto& parentRelationship = m_Registry.get<EntityRelationshipComponent>(relationship.parent);
            if (parentRelationship.firstChild == entity) { parentRelationship.firstChild = relationship.nextSibling; }
            parentRelationship.childCount--;
        }
        if (relationship.childCount > 0)
        {
            auto firstChild = relationship.firstChild;
            auto currentChild = firstChild;
            auto lastChild = relationship.firstChild;

            while (currentChild != entt::null)
            {
                auto& childRelationship = m_Registry.get<EntityRelationshipComponent>(currentChild);
                childRelationship.parent = relationship.parent;
                lastChild = currentChild;
                currentChild = childRelationship.nextSibling;
            }

            if (relationship.parent != entt::null)
            {
                auto& parentRelationship = m_Registry.get<EntityRelationshipComponent>(relationship.parent);
                auto parentFirstChild = parentRelationship.firstChild;
                if (parentFirstChild != entt::null)
                {
                    auto& parentFirstChildRelationship = m_Registry.get<EntityRelationshipComponent>(parentFirstChild);
                    parentFirstChildRelationship.prevSibling = lastChild;
                }
                parentRelationship.firstChild = firstChild;
                parentRelationship.childCount += relationship.childCount;
            }
        }
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

            auto view = m_Registry.view<ScriptComponent>();
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
                auto view = m_Registry.view<ScriptComponent>();
                for (auto e : view)
                {
                    Entity entity = {e, this};
                    ScriptEngine::OnUpdateEntity(entity, ts);
                }

                auto scriptView = m_Registry.view<NativeScriptComponent>();

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
        if (m_ViewportWidth == width && m_ViewportHeight == height) return;

        m_ViewportWidth = width;
        m_ViewportHeight = height;

        m_ViewportCamera.SetViewportSizeKeepFovY(width, height);
    }

    void Scene::Step(int frames) { m_StepFrames = frames; }

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
        auto view = m_Registry.view<TagComponent>();
        for (auto entity : view)
        {
            const TagComponent& tc = view.get<TagComponent>(entity);
            if (tc.tag == name) return Entity{entity, this};
        }
        return {};
    }

    Entity Scene::GetEntityByUUID(UUID uuid)
    {
        // TODO(Yan): Maybe should be assert
        if (m_EntityMap.contains(uuid)) return {m_EntityMap.at(uuid), this};

        return {};
    }

    std::vector<SceneCameraView> Scene::GetAllCameras()
    {
        std::vector<SceneCameraView> cameras;

        SceneCameraView editorViewCamera{};
        editorViewCamera.camera = &m_ViewportCamera;
        editorViewCamera.transform = m_ViewportCameraController.GetCameraTransform();
        editorViewCamera.entityUUID = UUID(-1);
        editorViewCamera.renderTextureUUID = UUID(-1);
        editorViewCamera.isEditorView = true;
        editorViewCamera.isPrimary = false;
        editorViewCamera.isViewportCamera = false;
        editorViewCamera.priority = 0;
        cameras.push_back(editorViewCamera);

        auto view = m_Registry.view<CameraComponent, IDComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto& cameraComponent = view.get<CameraComponent>(entity);
            auto& idComponent = view.get<IDComponent>(entity);
            auto& transformComponent = view.get<TransformComponent>(entity);

            SceneCameraView sceneCamera{};
            sceneCamera.camera = &cameraComponent.camera;
            sceneCamera.transform.translation = transformComponent.translation;
            sceneCamera.transform.rotation = transformComponent.rotation;
            sceneCamera.transform.scale = transformComponent.scale;
            sceneCamera.entityUUID = idComponent.ID;
            sceneCamera.renderTextureUUID = cameraComponent.renderTextureUUID;
            sceneCamera.isEditorView = false;
            sceneCamera.isPrimary = cameraComponent.isPrimary;
            sceneCamera.isViewportCamera = cameraComponent.isViewportCamera;
            sceneCamera.priority = cameraComponent.priority;
            cameras.push_back(sceneCamera);
        }

        return cameras;
    }

    std::vector<RenderSceneUpdatePayload> Scene::GetInitialRenderSceneUpdatePayloads() const
    {
        std::vector<RenderSceneUpdatePayload> payloads;

        auto view = m_Registry.view<MeshRendererComponent, IDComponent>();
        for (auto entity : view)
        {
            const auto& meshRenderer = view.get<MeshRendererComponent>(entity);
            const auto& idComponent = view.get<IDComponent>(entity);

            RenderSceneUpdatePayload payload{};
            payload.entity = idComponent.ID;
            payload.type = RenderSceneUpdatePayload::Type::Add;
            payload.add.renderObject.transform = GetWorldTransform(entity);
            payload.add.renderObject.material = meshRenderer.materialUUID;
            payload.add.renderObject.mesh = meshRenderer.meshUUID;
            payload.add.renderObject.entity = idComponent.ID;
            payload.add.renderObject.enttEntity = static_cast<uint32_t>(entity);
            payloads.push_back(payload);
        }

        return payloads;
    }

    std::vector<UUID> Scene::GetInitialAssetUUIDs() const
    {
        auto meshRendererEntities = m_Registry.view<MeshRendererComponent>();
        std::vector<UUID> uuids;
        for (auto entity : meshRendererEntities)
        {
            const auto& comp = meshRendererEntities.get<MeshRendererComponent>(entity);
            if (comp.meshUUID != UUID(-1)) { uuids.push_back(comp.meshUUID); }
            if (comp.materialUUID != UUID(-1)) { uuids.push_back(comp.materialUUID); }
        }
        return uuids;
    }

    glm::mat4 Scene::GetWorldTransform(entt::entity entity) const
    {
        const auto& relationship = m_Registry.get<EntityRelationshipComponent>(entity);
        const auto localTransform = BuildTransformMatrix(m_Registry.get<TransformComponent>(entity));
        if (relationship.parent == entt::null) { return localTransform; }

        return GetWorldTransform(relationship.parent) * localTransform;
    }

    void Scene::AddTransformPayloadsForSubtree(entt::entity entity, const glm::mat4& globalTransform)
    {
        if (m_Registry.any_of<MeshRendererComponent>(entity))
        {
            RenderSceneUpdatePayload payload{};
            payload.entity = m_Registry.get<IDComponent>(entity).ID;
            payload.type = RenderSceneUpdatePayload::Type::ChangeTransform;
            payload.changeTransform.transform = globalTransform;
            AddToRenderSceneUpdatePayload(payload);
        }

        const auto& relationship = m_Registry.get<EntityRelationshipComponent>(entity);
        auto child = relationship.firstChild;
        while (child != entt::null)
        {
            const auto childGlobalTransform =
                globalTransform * BuildTransformMatrix(m_Registry.get<TransformComponent>(child));
            AddTransformPayloadsForSubtree(child, childGlobalTransform);
            child = m_Registry.get<EntityRelationshipComponent>(child).nextSibling;
        }
    }

    template <typename T> void Scene::OnComponentAdded(Entity entity, T& component) { static_assert(sizeof(T) == 0); }

    template <> void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component) {}

    template <> void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component) {}

    template <>
    void Scene::OnComponentAdded<EntityRelationshipComponent>(Entity entity, EntityRelationshipComponent& component)
    {}

    template <> void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component) {}

    template <> void Scene::OnComponentAdded<MeshRendererComponent>(Entity entity, MeshRendererComponent& component) {}

    template <> void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component) {}

    template <> void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component) {}

    template <> void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component) {}
} // namespace Hazel