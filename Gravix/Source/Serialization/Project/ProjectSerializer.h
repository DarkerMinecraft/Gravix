#pragma once

#ifdef GRAVIX_EDITOR_BUILD

#include "Project/Project.h"
#include <filesystem>

namespace Gravix
{
	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref<Project> project)
			: m_Project(project) {
		}

		void Serialize(const std::filesystem::path& path);
		bool Deserialize(const std::filesystem::path& path);
	private:
		Ref<Project> m_Project;
	};

}

#endif // GRAVIX_EDITOR_BUILD