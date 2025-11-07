#include "pch.h"
#include "Project.h"

#include "Serialization/Project/ProjectSerializer.h"

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
			project->GetConfig().StartScene = project->GetConfig().AssetDirectory / project->GetConfig().StartScene;

			s_ActiveProject = project;

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