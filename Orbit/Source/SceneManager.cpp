#include "SceneManager.h"

#include "Project/Project.h"
#include "Asset/AssetManager.h"
#include "Serialization/Scene/SceneSerializer.h"
#include "Core/Log.h"
#include "Core/Application.h"

namespace Gravix
{

	bool SceneManager::SaveActiveScene()
	{
		// Check if we have a valid active scene
		if (!m_ActiveScene)
		{
			GX_CORE_ERROR("No active scene to save!");
			return false;
		}

		// Check if the scene has a valid asset handle and file path
		if (m_ActiveSceneHandle == 0 || AssetManager::GetAssetType(m_ActiveSceneHandle) != AssetType::Scene)
		{
			GX_CORE_WARN("Scene doesn't have a valid file path. Cannot save without a file location.");
			// TODO: Implement SaveSceneAs dialog here
			return false;
		}

		// Get the scene file path
		const auto& filePath = Project::GetAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetAssetFilePath(m_ActiveSceneHandle);

		// Save the scene
		SceneSerializer serializer(m_ActiveScene);
		serializer.Serialize(filePath);
		GX_CORE_INFO("Saved scene to: {0}", filePath.string());

		m_SceneDirty = false;

		// Notify that scene is no longer dirty
		if (m_OnSceneDirty)
			m_OnSceneDirty();

		return true;
	}

	bool SceneManager::OpenScene(AssetHandle handle, bool deserialize)
	{
		if (AssetManager::GetAssetType(handle) != AssetType::Scene)
		{
			GX_CORE_ERROR("Asset with handle {0} is not a scene!", static_cast<uint64_t>(handle));
			return false;
		}

		// Get scene file path
		const auto& assetManager = Project::GetActive()->GetEditorAssetManager();
		const auto& metadata = assetManager->GetAssetMetadata(handle);
		const auto& filePath = Project::GetAssetDirectory() / metadata.FilePath;

		Ref<Scene> scene = nullptr;

		// If we need to deserialize or if the asset isn't loaded yet, load synchronously
		if (deserialize || !AssetManager::IsAssetLoaded(handle))
		{
			if (std::filesystem::exists(filePath))
			{
				// Create a new scene and deserialize it from file
				scene = CreateRef<Scene>();
				SceneSerializer serializer(scene);
				serializer.Deserialize(filePath);
				GX_CORE_INFO("Loaded scene from: {0}", filePath.string());
			}
			else
			{
				GX_CORE_ERROR("Scene file not found: {0}", filePath.string());
				return false;
			}
		}
		else
		{
			// Try to get the already-loaded scene asset
			scene = AssetManager::GetAsset<Scene>(handle);

			if (!scene)
			{
				// Scene is loading asynchronously, track it so we can auto-load when ready
				m_PendingSceneHandle = handle;
				GX_CORE_INFO("Scene {0} is loading asynchronously, will auto-switch when ready", static_cast<uint64_t>(handle));
				return false;
			}
		}

		// Update the active scene
		if (scene)
		{
			// Wait for GPU to finish before destroying the old scene's resources
			if (m_ActiveScene)
			{
				Application::Get().GetWindow().GetDevice()->WaitIdle();
			}

			m_ActiveSceneHandle = handle;
			m_EditorScene = scene;
			m_ActiveScene = m_EditorScene;
			m_SceneDirty = false;

			if (m_OnSceneChanged)
				m_OnSceneChanged();

			return true;
		}

		return false;
	}

	Ref<Scene> SceneManager::LoadStartScene(const glm::vec2& viewportSize)
	{
		// Always create a default empty scene to prevent nullptr issues
		if (!m_ActiveScene)
		{
			m_ActiveScene = CreateRef<Scene>();
		}

		// Try to open the start scene if it's valid
		AssetHandle startScene = Project::GetActive()->GetConfig().StartScene;
		if (startScene != 0 && AssetManager::GetAssetType(startScene) == AssetType::Scene)
		{
			// Load scene synchronously during initialization to ensure it's ready
			const auto& assetManager = Project::GetActive()->GetEditorAssetManager();
			const auto& metadata = assetManager->GetAssetMetadata(startScene);
			const auto& filePath = Project::GetAssetDirectory() / metadata.FilePath;

			if (std::filesystem::exists(filePath))
			{
				// Create and deserialize the scene
				m_EditorScene = CreateRef<Scene>();
				SceneSerializer serializer(m_EditorScene);
				serializer.Deserialize(filePath);

				m_ActiveSceneHandle = startScene;
				m_EditorScene->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

				GX_CORE_INFO("Loaded start scene: {0}", filePath.string());
			}
			else
			{
				GX_CORE_WARN("Start scene file not found: {0}", filePath.string());
			}
		}

		m_SceneDirty = false;

		if (m_OnSceneChanged)
			m_OnSceneChanged();

		m_ActiveScene = m_EditorScene;

		return m_ActiveScene;
	}

	void SceneManager::Play()
	{
		m_SceneState = SceneState::Play;

		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnRuntimeStart();

		if (m_OnScenePlay)
			m_OnScenePlay();
	}

	void SceneManager::Stop()
	{
		m_SceneState = SceneState::Edit;
		m_ActiveScene->OnRuntimeStop();
		m_ActiveScene = m_EditorScene;

		if (m_OnSceneStop)
			m_OnSceneStop();
	}

	void SceneManager::SetActiveScene(Ref<Scene> scene, AssetHandle handle)
	{
		// Wait for GPU to finish before destroying the old scene's resources
		if (m_ActiveScene)
		{
			Application::Get().GetWindow().GetDevice()->WaitIdle();
		}

		m_ActiveScene = scene;
		m_ActiveSceneHandle = handle;

		if (m_OnSceneChanged)
			m_OnSceneChanged();
	}

	void SceneManager::MarkSceneDirty()
	{
		if (!m_SceneDirty)
		{
			m_SceneDirty = true;

			if (m_OnSceneDirty)
				m_OnSceneDirty();
		}
	}

}
