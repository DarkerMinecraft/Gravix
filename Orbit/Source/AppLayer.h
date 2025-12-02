#pragma once

#include "Core/Gravix.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ProjectSettingsPanel.h"

#include "ProjectManager.h"
#include "SceneManager.h"

#include "UI/EditorMenuBar.h"
#include "UI/EditorToolbar.h"
#include "Utils/KeyboardShortcutHandler.h"

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
		void RefreshContentBrowser();

		void UISettings();
		void OnOverlayRender(Command& cmd);
	private:
		Ref<Framebuffer> m_MSAAFramebuffer;
		Ref<Framebuffer> m_FinalFramebuffer;

		EditorCamera m_EditorCamera;
		glm::vec2 m_LastViewportSize = { 0.0f, 0.0f };

		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		InspectorPanel m_InspectorPanel;
		ViewportPanel m_ViewportPanel;
		std::optional<ContentBrowserPanel> m_ContentBrowserPanel;
		ProjectSettingsPanel m_ProjectSettingsPanel;

		// UI Components
		EditorMenuBar m_MenuBar;
		EditorToolbar m_Toolbar;

		// Managers
		ProjectManager m_ProjectManager;
		SceneManager m_SceneManager;
		KeyboardShortcutHandler m_ShortcutHandler;

		bool m_ShowPhysicsColliders = false;
		bool m_ProjectInitialized = false;
	};

}