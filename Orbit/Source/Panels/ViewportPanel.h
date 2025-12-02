#pragma once

#include "Renderer/Generic/Types/Framebuffer.h"
#include "Renderer/Generic/Types/Texture.h"
#include "Scene/EditorCamera.h"
#include "SceneHierarchyPanel.h"
#include "Core/Window.h"

#include "Events/KeyEvents.h"


namespace Gravix
{

	class AppLayer;
	class SceneManager;

	class ViewportPanel
	{
	public:
		ViewportPanel() = default;
		ViewportPanel(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex);
		~ViewportPanel() = default;

		void OnEvent(Event& e);
		void OnImGuiRender();

		void SetFramebuffer(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex) { m_Framebuffer = framebuffer; m_RenderIndex = renderIndex; }
		void SetEditorCamera(EditorCamera* camera) { m_EditorCamera = camera; }
		void SetSceneHierarchyPanel(SceneHierarchyPanel* panel) { m_SceneHierarchyPanel = panel; }
		void ResizeFramebuffer() { if(m_Framebuffer != nullptr) m_Framebuffer->Resize(m_ViewportSize.x, m_ViewportSize.y); }

		void SetAppLayer(AppLayer* appLayer) { m_AppLayer = appLayer; }
		void SetSceneManager(SceneManager* sceneManager) { m_SceneManager = sceneManager; }
		void LoadIcons();

		bool IsViewportValid() const { return m_ViewportSize.x > 0 && m_ViewportSize.y > 0; }
		bool IsViewportHovered() const { return m_ViewportHovered; }
		bool IsViewportFocused() const { return m_ViewportFocused; }

		void SetImGuizmoNone() { m_GuizmoType = -1; }

		Entity GetHoveredEntity() const { return m_HoveredEntity; }

		void UpdateViewport();

		const glm::vec2& GetViewportSize() const { return m_ViewportSize; }
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		void DrawToolbarOverlay();
		void DrawGizmoModeButtons();
	private:
		Ref<Framebuffer> m_Framebuffer;
		glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };
		std::array<glm::vec2, 2> m_ViewportBounds;

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		int m_GuizmoType = -1;

		EditorCamera* m_EditorCamera;
		SceneHierarchyPanel* m_SceneHierarchyPanel;

		AppLayer* m_AppLayer;
		SceneManager* m_SceneManager = nullptr;

		Entity m_HoveredEntity;
		CursorMode m_CurrentCursorMode = CursorMode::Normal;

		uint32_t m_RenderIndex = 0;

		// Toolbar icons
		Ref<Texture2D> m_IconPlay;
		Ref<Texture2D> m_IconStop;
	};
}