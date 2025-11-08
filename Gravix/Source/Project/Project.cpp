#include "pch.h"
#include "Project.h"

#include "Serialization/Project/ProjectSerializer.h"
#include "Asset/EditorAssetManager.h"

namespace Gravix 
{

	Ref<Project> Project::New()
	{
		s_ActiveProject = CreateRef<Project>();

		// Initialize with default values but no working directory
		s_ActiveProject->m_Config.StartScene = 0; // Null handle

		// Initialize asset manager
		Ref<EditorAssetManager> editorAssetManager = CreateRef<EditorAssetManager>();
		s_ActiveProject->m_AssetManager = editorAssetManager;

		return s_ActiveProject;
	}

	Ref<Project> Project::New(const std::filesystem::path& workingDirectory)
	{
		s_ActiveProject = CreateRef<Project>();

		// Set working directory
		s_ActiveProject->m_WorkingDirectory = workingDirectory;

		// Set default configuration values
		s_ActiveProject->m_Config.Name = "Untitled";
		s_ActiveProject->m_Config.StartScene = 0; // Null handle
		s_ActiveProject->m_Config.AssetDirectory = workingDirectory / "Assets";
		s_ActiveProject->m_Config.LibraryDirectory = workingDirectory / "Library";
		s_ActiveProject->m_Config.ScriptPath = workingDirectory / "Scripts";

		// Create directories if they don't exist
		std::filesystem::create_directories(s_ActiveProject->m_Config.AssetDirectory);
		std::filesystem::create_directories(s_ActiveProject->m_Config.LibraryDirectory);
		std::filesystem::create_directories(s_ActiveProject->m_Config.ScriptPath);

		GX_CORE_INFO("Created project directories:");
		GX_CORE_INFO("  Assets:  {}", s_ActiveProject->m_Config.AssetDirectory.string());
		GX_CORE_INFO("  Library: {}", s_ActiveProject->m_Config.LibraryDirectory.string());
		GX_CORE_INFO("  Scripts: {}", s_ActiveProject->m_Config.ScriptPath.string());

		// Initialize asset manager
		Ref<EditorAssetManager> editorAssetManager = CreateRef<EditorAssetManager>();
		s_ActiveProject->m_AssetManager = editorAssetManager;

		return s_ActiveProject;
	}

	Ref<Project> Project::Load(const std::filesystem::path& path)
	{
		Ref<Project> project = CreateRef<Project>();
		ProjectSerializer serializer(project);
		if (serializer.Deserialize(path))
		{
			// Convert relative paths to absolute paths
			project->GetConfig().AssetDirectory = path.parent_path() / project->GetConfig().AssetDirectory;
			project->GetConfig().LibraryDirectory = path.parent_path() / project->GetConfig().LibraryDirectory;
			project->GetConfig().ScriptPath = path.parent_path() / project->GetConfig().ScriptPath;

			// Create directories if they don't exist
			std::filesystem::create_directories(project->GetConfig().AssetDirectory);
			std::filesystem::create_directories(project->GetConfig().LibraryDirectory);
			std::filesystem::create_directories(project->GetConfig().ScriptPath);

			s_ActiveProject = project;
			Ref<EditorAssetManager> editorAssetManager = CreateRef<EditorAssetManager>();
			s_ActiveProject->m_AssetManager = editorAssetManager;
			editorAssetManager->DeserializeAssetRegistry();
			s_ActiveProject->m_WorkingDirectory = path.parent_path();

			return s_ActiveProject;
		}

		return nullptr;
	}

	void Project::SaveActive(const std::filesystem::path& path)
	{
		ProjectSerializer serializer(s_ActiveProject);
		serializer.Serialize(path);
	}

}