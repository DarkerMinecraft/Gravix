#include "ProjectManager.h"

#include "Utils/PlatformUtils.h"
#include "Core/Log.h"

namespace Gravix
{

	bool ProjectManager::CreateNewProject()
	{
		// Prompt user to select a folder for the new project
		std::filesystem::path projectFolder = FileDialogs::OpenFolder("Select Project Location");
		if (projectFolder.empty())
			return false;

		// Check if a project file already exists in this folder
		std::filesystem::path projectPath = projectFolder / ".orbproj";
		if (std::filesystem::exists(projectPath))
		{
			// Project already exists, open it instead of creating a new one
			GX_CORE_INFO("Project file already exists at: {0}, opening existing project", projectPath.string());
			m_ActiveProjectPath = projectPath;

			if (Project::Load(m_ActiveProjectPath))
			{
				if (m_OnProjectLoaded)
					m_OnProjectLoaded();

				m_ShowStartupDialog = false;
				return true;
			}
			return false;
		}

		// Create the project with default directories
		Project::New(projectFolder);

		// Set the project path and save it
		m_ActiveProjectPath = projectPath;
		Project::SaveActive(m_ActiveProjectPath);

		GX_CORE_INFO("New project created at: {0}", projectFolder.string());

		if (m_OnProjectCreated)
			m_OnProjectCreated();

		m_ShowStartupDialog = false;
		return true;
	}

	bool ProjectManager::OpenProject()
	{
		m_ActiveProjectPath = FileDialogs::OpenFile("Orbit Project (*.orbproj)\0*.orbproj\0");
		if (m_ActiveProjectPath.empty())
			return false;

		if (Project::Load(m_ActiveProjectPath))
		{
			if (m_OnProjectLoaded)
				m_OnProjectLoaded();

			m_ShowStartupDialog = false;
			return true;
		}

		return false;
	}

	bool ProjectManager::SaveActiveProject()
	{
		if (m_ActiveProjectPath.empty())
		{
			return SaveActiveProjectAs();
		}

		Project::SaveActive(m_ActiveProjectPath);
		return true;
	}

	bool ProjectManager::SaveActiveProjectAs()
	{
		std::filesystem::path filePath = FileDialogs::SaveFile("Orbit Project (*.orbproj)\0*.orbproj\0");
		if (filePath.empty())
			return false;

		m_ActiveProjectPath = filePath;
		Project::SaveActive(m_ActiveProjectPath);
		return true;
	}

}
