#pragma once

#include <functional>

namespace Gravix
{
	class ProjectManager;
	class SceneManager;
	class ProjectSettingsPanel;
	class ContentBrowserPanel;

	class EditorMenuBar
	{
	public:
		EditorMenuBar() = default;
		~EditorMenuBar() = default;

		void SetProjectManager(ProjectManager* projectManager) { m_ProjectManager = projectManager; }
		void SetSceneManager(SceneManager* sceneManager) { m_SceneManager = sceneManager; }
		void SetProjectSettingsPanel(ProjectSettingsPanel* panel) { m_ProjectSettingsPanel = panel; }
		void SetContentBrowserPanel(ContentBrowserPanel* panel) { m_ContentBrowserPanel = panel; }

		void SetOnProjectCreatedCallback(std::function<void()> callback) { m_OnProjectCreated = callback; }
		void SetOnProjectOpenedCallback(std::function<void()> callback) { m_OnProjectOpened = callback; }

		void OnImGuiRender();

	private:
		ProjectManager* m_ProjectManager = nullptr;
		SceneManager* m_SceneManager = nullptr;
		ProjectSettingsPanel* m_ProjectSettingsPanel = nullptr;
		ContentBrowserPanel* m_ContentBrowserPanel = nullptr;

		std::function<void()> m_OnProjectCreated;
		std::function<void()> m_OnProjectOpened;
	};

}
