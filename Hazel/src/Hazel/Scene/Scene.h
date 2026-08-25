#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Core/UUID.h"
// ======== Aster Modify Begin ========
#include "Hazel/Renderer/Camera.h"
#include "Hazel/Renderer/RenderScene.h"
#include "Transform.h"
#include "ViewportCameraController.h"

#include <entt.hpp>

// ======== Aster Modify End ========

namespace Hazel
{
    class Entity;

    // ======== Aster Modify Begin ========
    struct SceneCameraView
    {
        Camera* camera = nullptr;
        Aster::Transform transform;
        UUID entityUUID = UUID(-1);
        UUID renderTextureUUID = UUID(-1);
        bool isEditorView = false;
        bool isPrimary = false;
        bool isViewportCamera = false;
        int priority = 0;
    };

    // ======== Aster Modify End ========

    class Scene
    {
      public:
        Scene();
        ~Scene();

        static Ref<Scene> Copy(Ref<Scene> other);

        Entity CreateEntity(const std::string& name = std::string());
        Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
        void DestroyEntity(Entity entity);

        void OnRuntimeStart();
        void OnRuntimeStop();

        void OnUpdateRuntime(Timestep ts);
        void OnUpdateSimulation(Timestep ts);
        void OnViewportResize(uint32_t width, uint32_t height);

        Entity DuplicateEntity(Entity entity);

        Entity FindEntityByName(std::string_view name);
        Entity GetEntityByUUID(UUID uuid);
        // ======== Aster Modify Begin ========
        std::vector<SceneCameraView> GetAllCameras();
        std::vector<Aster::RenderSceneUpdatePayload> GetInitialRenderSceneUpdatePayloads() const;

        std::vector<UUID> GetInitialAssetUUIDs() const;

        auto& GetMapUUIDToEntity() { return m_EntityMap; }

        const Camera& GetViewportCamera() const { return m_ViewportCamera; }

        SceneCameraView GetSceneViewportCamera()
        {
            SceneCameraView ret{};
            ret.camera = &m_ViewportCamera;
            ret.entityUUID = UUID(-1);
            ret.renderTextureUUID = UUID(-1);
            ret.isEditorView = true;
            ret.isPrimary = false;
            ret.isViewportCamera = true;
            ret.transform = m_ViewportCameraController.GetCameraTransform();
            return ret;
        }

        void SetViewportCamera(const Camera& camera) { m_ViewportCamera = camera; }

        // ======== Aster Modify End ========

        bool IsRunning() const { return m_IsRunning; }

        bool IsPaused() const { return m_IsPaused; }

        void SetPaused(bool paused) { m_IsPaused = paused; }

        void Step(int frames = 1);

        // ======== Aster Modify Begin ========
        std::string GetName() const { return m_Name; }

        void SetName(const std::string& name) { m_Name = name; }

        // ======== Aster Modify End ========

        template <typename... Components> auto GetAllEntitiesWith() { return m_Registry.view<Components...>(); }

        // ======== Aster Modify Begin ========
        void AddToRenderSceneUpdatePayload(Aster::RenderSceneUpdatePayload payload)
        { m_RenderSceneUpdatePayloads.push_back(payload); }

        std::vector<Aster::RenderSceneUpdatePayload> GetRenderSceneUpdatePayloads()
        {
            auto payload = m_RenderSceneUpdatePayloads;
            m_RenderSceneUpdatePayloads.clear();
            return payload;
        }

        entt::registry& GetRegistry() { return m_Registry; }

        Aster::ViewportCameraController& GetViewportCameraController() { return m_ViewportCameraController; }

        // ======== Aster Modify End ========

      private:
        template <typename T> void OnComponentAdded(Entity entity, T& component);
        // ======== Aster Modify Begin ========
        glm::mat4 GetWorldTransform(entt::entity entity) const;
        void AddTransformPayloadsForSubtree(entt::entity entity, const glm::mat4& globalTransform);

        entt::registry m_Registry;
        uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
        // ======== Aster Modify End ========
        bool m_IsRunning = false;
        bool m_IsPaused = false;
        int m_StepFrames = 0;

        // ======== Aster Modify Begin ========
        Aster::ViewportCameraController m_ViewportCameraController;
        Camera m_ViewportCamera;

        std::string m_Name = "Untitled Scene";
        // ======== Aster Modify End ========

        std::unordered_map<UUID, entt::entity> m_EntityMap;

        // ======== Aster Modify Begin ========
        std::vector<Aster::RenderSceneUpdatePayload> m_RenderSceneUpdatePayloads;
        // ======== Aster Modify End ========

        friend class Entity;
        friend class SceneSerializer;
        friend class SceneHierarchyPanel;
    };
} // namespace Hazel
