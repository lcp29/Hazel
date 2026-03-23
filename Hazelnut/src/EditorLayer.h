#pragma once

#include "Hazel.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Hazel/Scene/ViewportCameraController.h"

namespace Hazel {

	class EditorLayer : public Layer
	{
	public:
		struct EditorUITexture
		{
			RHIImage* Image = nullptr;
			RHIImageView* View = nullptr;
			void* ImGuiTexture = nullptr;
		};

		EditorLayer(Renderer* renderer);
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OnOverlayRender();

		void RecreateDefaultRenderTextureData();
	    void RecreateObjectIDRenderData();
	    void OnObjectIDMapRender();
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
		void UI_Toolbar();
	private:
		// Editor textures
		RHISampler* m_UISampler = nullptr;
		RHISampler* m_CheckerboardSampler = nullptr;
		EditorUITexture m_IconPlay, m_IconPause, m_IconStep, m_IconStop;
		EditorUITexture m_ContentBrowserDirectoryIcon, m_ContentBrowserFileIcon;
		EditorUITexture m_CheckerboardTexture;
        std::vector<EditorUITexture> m_DefaultRenderTextureUIData;
		// Scenes
		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		std::filesystem::path m_EditorScenePath;
		Entity m_HoveredEntity;

	    // Rendering
	    Renderer* m_Renderer = nullptr;
	    RenderTexture* m_ObjectIDRenderTexture = nullptr;
	    std::vector<RHIBuffer*> m_ObjectIDRenderTextureBuffer;

	    // viewport related
		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };
		glm::vec2 m_ViewportBounds[2];
	    ViewportCameraController m_ViewportCameraController;

		int m_GizmoType = -1;

		enum class SceneState
		{
			Edit = 0, Play = 1
		};
		SceneState m_SceneState = SceneState::Edit;

		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		Scope<ContentBrowserPanel> m_ContentBrowserPanel;

	};

}
