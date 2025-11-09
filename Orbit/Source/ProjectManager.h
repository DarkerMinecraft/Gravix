#pragma once

#include "Project/Project.h"
#include "Scene/Scene.h"

#include <filesystem>
#include <functional>

namespace Gravix
{

	class ProjectManager
	{
	public:
		ProjectManager() = default;
		~ProjectManager() = default;

		// Project lifecycle
		bool CreateNewProject();
		bool OpenProject();
		bool SaveActiveProject();
		bool SaveActiveProjectAs();

		// Callbacks for when project operations complete
		void SetOnProjectLoadedCallback(std::function<void()> callback) { m_OnProjectLoaded = callback; }
		void SetOnProjectCreatedCallback(std::function<void()> callback) { m_OnProjectCreated = callback; }

		// Get the active project path
		const std::filesystem::path& GetActiveProjectPath() const { return m_ActiveProjectPath; }

		// Show startup dialog (returns true if dialog should be shown)
		bool ShouldShowStartupDialog() const { return m_ShowStartupDialog; }
		void SetShowStartupDialog(bool show) { m_ShowStartupDialog = show; }

	private:
		std::filesystem::path m_ActiveProjectPath;
		std::function<void()> m_OnProjectLoaded;
		std::function<void()> m_OnProjectCreated;
		bool m_ShowStartupDialog = true;
	};

}
