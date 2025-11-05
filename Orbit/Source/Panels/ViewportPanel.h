#pragma once

#include "Renderer/Generic/Types/Framebuffer.h"
#include "Scene/EditorCamera.h"
#include "SceneHierarchyPanel.h"

namespace Gravix
{
	class ViewportPanel
	{
	public:
		ViewportPanel() = default;
		ViewportPanel(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex);
		~ViewportPanel() = default;

		void OnImGuiRender();

		void SetFramebuffer(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex) { m_Framebuffer = framebuffer; m_RenderIndex = renderIndex; }
		void SetEditorCamera(EditorCamera* camera) { m_EditorCamera = camera; }
		void SetSceneHierarchyPanel(SceneHierarchyPanel* panel) { m_SceneHierarchyPanel = panel; }
		void ResizeFramebuffer() { m_Framebuffer->Resize(m_ViewportSize.x, m_ViewportSize.y); }

		bool IsViewportValid() const { return m_ViewportSize.x > 0 && m_ViewportSize.y > 0; }
		bool IsViewportHovered() const { return m_ViewportHovered; }
		bool IsViewportFocused() const { return m_ViewportFocused; }

		void GuizmoShortcuts();

		const glm::vec2& GetViewportSize() const { return m_ViewportSize; }
	private:
		Ref<Framebuffer> m_Framebuffer;
		glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		int m_GuizmoType = -1;

		EditorCamera* m_EditorCamera;
		SceneHierarchyPanel* m_SceneHierarchyPanel;

		uint32_t m_RenderIndex = 0;
	};
}