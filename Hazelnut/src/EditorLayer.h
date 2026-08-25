#pragma once

#include "Hazel.h"
// ======== Aster Modify Begin ========
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Scene/ViewportCameraController.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/PropertyPanel.h"
#include "Panels/SceneHierarchyPanel.h"

namespace Aster
{
    class GPURenderBufferAsset;
}

namespace Hazel
// ======== Aster Modify End ========
{
    class EditorLayer : public Layer
    {
      public:
        // ======== Aster Modify Begin ========
        constexpr static auto GizmoTranslateSnapString = "editor.Gizmo.TranslateSnap";
        constexpr static auto GizmoRotateSnapDegreesString = "editor.Gizmo.RotateSnapDegrees";
        constexpr static float DefaultGizmoTranslateSnap = 0.5f;
        constexpr static float DefaultGizmoRotateSnapDegrees = 45.0f;

        constexpr static int DefaultMouseObjectIDEventQueueSize = 1 << 2;

        struct EditorUITexture
        {
            Aster::RHIImage* Image = nullptr;
            Aster::RHIImageView* View = nullptr;
            void* ImGuiTexture = nullptr;
        };

        EditorLayer(Renderer* renderer);
        ~EditorLayer() override = default;
        // ======== Aster Modify End ========

        void OnAttach() override;
        void OnDetach() override;

        void OnUpdate(Timestep ts) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

      private:
        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

        void OnOverlayRender();

        // ======== Aster Modify Begin ========
        void RecreateDefaultRenderTextureData();
        void RecreateObjectIDRenderData();
        void OnObjectIDMapRender(uint32_t x, uint32_t y);
        void ResetProjectContext();
        bool HasOpenProject() const;
        void DrawProjectSelectionScreen();

        void NewProject();
        // ======== Aster Modify End ========
        bool OpenProject();
        void OpenProject(const std::filesystem::path& path);
        void SaveProject();

        void NewScene();
        void OpenScene();
        void OpenScene(const std::filesystem::path& path);
        void SaveScene();
        void SaveSceneAs();

        void SerializeScene(Ref<Scene> scene, const std::filesystem::path& path);

        void OnScenePlay();
        // ======== Aster Modify Begin ========
        void OnSceneStop();
        void OnScenePause();

        void OnDuplicateEntity();

        // UI Panels
        void UIDrawMenuBar();
        void UIDrawToolbar();
        void UIDrawViewportAndGizmos();

        // editor textures
        Aster::RHISampler* m_UISampler = nullptr;
        Aster::RHISampler* m_CheckerboardSampler = nullptr;
        EditorUITexture m_IconPlay, m_IconPause, m_IconStep, m_IconStop;
        EditorUITexture m_ContentBrowserDirectoryIcon, m_ContentBrowserFileIcon;
        EditorUITexture m_CheckerboardTexture;
        std::vector<EditorUITexture> m_DefaultRenderTextureUIData;
        // ======== Aster Modify End ========

        // scenes
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;
        std::filesystem::path m_EditorScenePath;
        // ======== Aster Modify Begin ========
        Entity m_ClickedEntity;

        // rendering
        Renderer* m_Renderer = nullptr;
        UUID m_ObjectIDRenderTextureUUID = UUID();
        std::mutex m_ObjectIDRenderMutex;
        std::vector<bool> m_ObjectIDNeedsPulling;
        std::vector<std::array<int32_t, 2>> m_ObjectIDPullPositions;
        uint32_t m_ObjectIDPullingQueueFront = 0;
        uint32_t m_ObjectIDPullingQueueBack = 0;
        std::unique_ptr<Aster::GPUAssetHandle> m_ObjectIDRenderTexture = nullptr;
        std::unique_ptr<Aster::GPUAssetHandle> m_ObjectIDDepthRenderTexture = nullptr;
        std::unique_ptr<Aster::GPUAssetHandle> m_ObjectIDRenderTextureBuffer = nullptr;
        // ======== Aster Modify End ========

        // viewport related
        bool m_ViewportFocused = false, m_ViewportHovered = false;
        // ======== Aster Modify Begin ========
        glm::vec2 m_ViewportSize = {1280.0f, 720.0f};
        // ======== Aster Modify End ========
        glm::vec2 m_ViewportBounds[2];

        int m_GizmoType = -1;

        enum class SceneState
        {
            Edit = 0,
            Play = 1
        };

        SceneState m_SceneState = SceneState::Edit;

        // Panels
        SceneHierarchyPanel m_SceneHierarchyPanel;
        Scope<ContentBrowserPanel> m_ContentBrowserPanel;
        // ======== Aster Modify Begin ========
        Aster::PropertyPanel m_PropertyPanel;
        uint64_t m_LastSceneHierarchySelectionVersion = 0;
        uint64_t m_LastContentBrowserSelectionVersion = 0;
        // ======== Aster Modify End ========
    };
} // namespace Hazel
