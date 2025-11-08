#pragma once

#include "Core/Core.h"
#include "Core/Log.h"

#include "Asset/AssetManagerBase.h"

#include "Asset/EditorAssetManager.h"
#include "Asset/RuntimeAssetManager.h"

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

		static std::filesystem::path& GetWorkingDirectory() 
		{
			GX_CORE_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject->m_WorkingDirectory;
		}

		ProjectConfig& GetConfig() { return m_Config; }

		static Ref<Project> GetActive() 
		{
			GX_CORE_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject;
		}

		Ref<AssetManagerBase> GetAssetManager() { return m_AssetManager; }
		Ref<EditorAssetManager> GetEditorAssetManager() { return Cast<EditorAssetManager>(m_AssetManager); }
		Ref<RuntimeAssetManager> GetRuntimeAssetManager() { return Cast<RuntimeAssetManager>(m_AssetManager); }

		static Ref<Project> New();
		static Ref<Project> Load(const std::filesystem::path& path);
		static void SaveActive(const std::filesystem::path& path);
	private:
		ProjectConfig m_Config;
		std::filesystem::path m_WorkingDirectory;

		Ref<AssetManagerBase> m_AssetManager;

		inline static Ref<Project> s_ActiveProject;
	};

}