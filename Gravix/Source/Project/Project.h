#pragma once

#include "Core/Core.h"
#include "Core/Log.h"

#include <filesystem>

namespace Gravix 
{
	
	struct ProjectConfig
	{
		std::string Name = "Untitled";
		
		std::filesystem::path StartScene;

		std::filesystem::path AssetDirectory;
		std::filesystem::path LibraryDirectory;
		std::filesystem::path ScriptPath;
	};

	class Project 
	{
	public:
		Project() = default;

		static std::filesystem::path& GetAssetDirectory()
		{
			GX_CORE_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject->m_Config.AssetDirectory;
		};

		static std::filesystem::path& GetLibraryDirectory()
		{
			GX_CORE_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject->m_Config.LibraryDirectory;
		};

		ProjectConfig& GetConfig() { return m_Config; }

		static ProjectConfig& GetActiveConfig() 
		{
			GX_CORE_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject->m_Config;
		};

		static Ref<Project> New();
		static Ref<Project> Load(const std::filesystem::path& path);
		static void SaveActive(const std::filesystem::path& path);
	private:
		ProjectConfig m_Config;

		inline static Ref<Project> s_ActiveProject;
	};

}