#include "pch.h"
#include "Project.h"

#include "Serialization/Project/ProjectSerializer.h"
#include "Asset/EditorAssetManager.h"

namespace Gravix 
{

	Ref<Project> Project::New()
	{
		s_ActiveProject = CreateRef<Project>();
		return s_ActiveProject;
	}

	Ref<Project> Project::Load(const std::filesystem::path& path)
	{
		Ref<Project> project = CreateRef<Project>();
		ProjectSerializer serializer(project);
		if (serializer.Deserialize(path))
		{
			project->GetConfig().AssetDirectory = path.parent_path() / project->GetConfig().AssetDirectory;
			project->GetConfig().LibraryDirectory = path.parent_path() / project->GetConfig().LibraryDirectory;

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