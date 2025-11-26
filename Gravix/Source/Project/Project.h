#pragma once

#include "Core/Core.h"
#include "Core/RefCounted.h"
#include "Core/Log.h"

#include "Asset/AssetManagerBase.h"

#include "Asset/EditorAssetManager.h"
#include "Asset/RuntimeAssetManager.h"

#include "Asset/Asset.h"

#include <filesystem>
#include <glm/glm.hpp>

namespace Gravix
{

	struct PhysicsSettings
	{
		glm::vec2 Gravity = { 0.0f, -9.8f };
		float RestitutionThreshold = 2.4f;  // Box2D default: 1.0 m/s (we use 2.4 for pixels)
	};

	struct ProjectConfig
	{
		std::string Name = "Untitled";

		AssetHandle StartScene;

		std::filesystem::path AssetDirectory;
		std::filesystem::path LibraryDirectory;
		std::filesystem::path ScriptPath;

		PhysicsSettings Physics;
	};

	class Project : public RefCounted
	{
	public:
		Project() = default;

		static std::filesystem::path& GetAssetDirectory()
		{
			GX_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject->m_Config.AssetDirectory;
		};

		static std::filesystem::path& GetLibraryDirectory()
		{
			GX_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject->m_Config.LibraryDirectory;
		};

		static std::filesystem::path& GetWorkingDirectory() 
		{
			GX_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject->m_WorkingDirectory;
		}

		ProjectConfig& GetConfig() { return m_Config; }

		static Ref<Project> GetActive() 
		{
			GX_ASSERT(s_ActiveProject, "No active project!");
			return s_ActiveProject;
		}

		static bool HasActiveProject()
		{
			return s_ActiveProject != nullptr;
		}

		Ref<AssetManagerBase> GetAssetManager() { return m_AssetManager; }
		Ref<EditorAssetManager> GetEditorAssetManager() { return Cast<EditorAssetManager>(m_AssetManager); }
		Ref<RuntimeAssetManager> GetRuntimeAssetManager() { return Cast<RuntimeAssetManager>(m_AssetManager); }

		static Ref<Project> New();
		static Ref<Project> New(const std::filesystem::path& workingDirectory);
		static Ref<Project> Load(const std::filesystem::path& path);
		static void SaveActive(const std::filesystem::path& path);
	private:
		ProjectConfig m_Config;
		std::filesystem::path m_WorkingDirectory;

		Ref<AssetManagerBase> m_AssetManager;

		inline static Ref<Project> s_ActiveProject;
	};

}