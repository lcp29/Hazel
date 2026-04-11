#pragma once

#include "Hazel.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Panels/PropertyPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Hazel/Scene/ViewportCameraController.h"

namespace Hazel
{
    class GPURenderBufferAsset;

    class EditorLayer : public Layer
    {
    public:
        constexpr static auto GizmoTranslateSnapString = "editor.Gizmo.TranslateSnap";
        constexpr static auto GizmoRotateSnapDegreesString = "editor.Gizmo.RotateSnapDegrees";
        constexpr static float DefaultGizmoTranslateSnap = 0.5f;
        constexpr static float DefaultGizmoRotateSnapDegrees = 45.0f;

        constexpr static int DefaultMouseObjectIDEventQueueSize = 1 << 2;

        struct EditorUITexture
        {
            RHIImage* Image = nullptr;
            RHIImageView* View = nullptr;
            void* ImGuiTexture = nullptr;
        };

        EditorLayer(Renderer* renderer);
        ~EditorLayer() override = default;

        void OnAttach() override;
        void OnDetach() override;

        void OnUpdate(Timestep ts) override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

        void OnOverlayRender();

        void RecreateDefaultRenderTextureData();
        void RecreateObjectIDRenderData();
        void OnObjectIDMapRender(uint32_t x, uint32_t y);
        void ResetProjectContext();
        bool HasOpenProject() const;
        void DrawProjectSelectionScreen();

        void NewProject();
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
        void OnSceneStop();
        void OnScenePause();

        void OnDuplicateEntity();

        // UI Panels
        void UIToolbar();

        // editor textures
        RHISampler* m_UISampler = nullptr;
        RHISampler* m_CheckerboardSampler = nullptr;
        EditorUITexture m_IconPlay, m_IconPause, m_IconStep, m_IconStop;
        EditorUITexture m_ContentBrowserDirectoryIcon, m_ContentBrowserFileIcon;
        EditorUITexture m_CheckerboardTexture;
        std::vector<EditorUITexture> m_DefaultRenderTextureUIData;

        // scenes
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;
        std::filesystem::path m_EditorScenePath;
        Entity m_ClickedEntity;

        // rendering
        Renderer* m_Renderer = nullptr;
        UUID m_ObjectIDRenderTextureUUID = UUID();
        std::mutex m_ObjectIDRenderMutex;
        std::vector<bool> m_ObjectIDNeedsPulling;
        std::vector<std::array<int32_t, 2>> m_ObjectIDPullPositions;
        uint32_t m_ObjectIDPullingQueueFront;
        uint32_t m_ObjectIDPullingQueueBack;
        std::unique_ptr<GPUAssetHandle> m_ObjectIDRenderTexture = nullptr;
        std::unique_ptr<GPUAssetHandle> m_ObjectIDDepthRenderTexture = nullptr;
        std::unique_ptr<GPUAssetHandle> m_ObjectIDRenderTextureBuffer = nullptr;

        // viewport related
        bool m_ViewportFocused = false, m_ViewportHovered = false;
        glm::vec2 m_ViewportSize = {1280.0f, 720.0f};
        glm::vec2 m_ViewportBounds[2];

        int m_GizmoType = -1;

        enum class SceneState
        {
            Edit = 0, Play = 1
        };

        SceneState m_SceneState = SceneState::Edit;

        // Panels
        SceneHierarchyPanel m_SceneHierarchyPanel;
        Scope<ContentBrowserPanel> m_ContentBrowserPanel;
        PropertyPanel m_PropertyPanel;
        uint64_t m_LastSceneHierarchySelectionVersion = 0;
        uint64_t m_LastContentBrowserSelectionVersion = 0;
    };
}