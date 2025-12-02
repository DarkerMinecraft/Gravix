#pragma once

#include "Events/KeyEvents.h"

#include <functional>

namespace Gravix
{
	class ProjectManager;
	class SceneManager;
	class SceneHierarchyPanel;

	class KeyboardShortcutHandler
	{
	public:
		KeyboardShortcutHandler() = default;
		~KeyboardShortcutHandler() = default;

		void SetProjectManager(ProjectManager* projectManager) { m_ProjectManager = projectManager; }
		void SetSceneManager(SceneManager* sceneManager) { m_SceneManager = sceneManager; }
		void SetSceneHierarchyPanel(SceneHierarchyPanel* panel) { m_SceneHierarchyPanel = panel; }

		void SetOnProjectCreatedCallback(std::function<void()> callback) { m_OnProjectCreated = callback; }
		void SetOnProjectOpenedCallback(std::function<void()> callback) { m_OnProjectOpened = callback; }

		bool HandleKeyPress(KeyPressedEvent& e);

	private:
		ProjectManager* m_ProjectManager = nullptr;
		SceneManager* m_SceneManager = nullptr;
		SceneHierarchyPanel* m_SceneHierarchyPanel = nullptr;

		std::function<void()> m_OnProjectCreated;
		std::function<void()> m_OnProjectOpened;

		bool HandleNewProject();
		bool HandleOpenProject();
		bool HandleSave(bool saveAs);
		bool HandleDuplicate();
	};

}
