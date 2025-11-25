#pragma once

#include "Core/Gravix.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ProjectSettingsPanel.h"

#include "ProjectManager.h"
#include "SceneManager.h"

#include <glm/glm.hpp>
#include <optional>

namespace Gravix
{

	class AppLayer : public Layer
	{
	public:
		AppLayer();
		virtual ~AppLayer();

		virtual void OnEvent(Event& event) override;

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnRender() override;

		virtual void OnImGuiRender() override;

		void MarkSceneDirty();
		void UpdateWindowTitle();
		void OpenScene(AssetHandle handle, bool deserialize = false);

		Ref<Scene> GetActiveScene() const { return m_SceneManager.GetActiveScene(); }
		AssetHandle GetActiveSceneHandle() const { return m_SceneManager.GetActiveSceneHandle(); }
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnFileDrop(WindowFileDropEvent& e);

		void InitializeProject();

		void UIToolbar();
		void UISettings();

		void OnOverlayRender(Command& cmd);
	private:
		Ref<Framebuffer> m_MSAAFramebuffer;
		Ref<Framebuffer> m_FinalFramebuffer;

		Ref<Texture2D> m_IconPlay;
		Ref<Texture2D> m_IconStop;

		EditorCamera m_EditorCamera;
		glm::vec2 m_LastViewportSize = { 0.0f, 0.0f };

		SceneHierarchyPanel m_SceneHierarchyPanel;
		InspectorPanel m_InspectorPanel;
		ViewportPanel m_ViewportPanel;
		std::optional<ContentBrowserPanel> m_ContentBrowserPanel;
		ProjectSettingsPanel m_ProjectSettingsPanel;

		bool m_ShowPhysicsColliders = false;

		// Managers
		ProjectManager m_ProjectManager;
		SceneManager m_SceneManager;

		bool m_ProjectInitialized = false;
	};

}