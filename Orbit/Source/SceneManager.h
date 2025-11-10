#pragma once

#include "Scene/Scene.h"
#include "Asset/Asset.h"
#include "Core/Core.h"

#include <filesystem>
#include <functional>
#include <glm/glm.hpp>

namespace Gravix
{

	enum class SceneState
	{
		Edit = 0,
		Play = 1
	};

	class SceneManager
	{
	public:
		SceneManager() = default;
		~SceneManager() = default;

		// Scene lifecycle
		bool SaveActiveScene();
		bool OpenScene(AssetHandle handle, bool deserialize = true);
		Ref<Scene> LoadStartScene(const glm::vec2& viewportSize);

		// Scene state
		void Play();
		void Stop();
		SceneState GetSceneState() const { return m_SceneState; }

		// Active scene management
		void SetActiveScene(Ref<Scene> scene, AssetHandle handle);
		Ref<Scene> GetActiveScene() const { return m_ActiveScene; }
		AssetHandle GetActiveSceneHandle() const { return m_ActiveSceneHandle; }

		// Pending scene (async loading)
		void SetPendingScene(AssetHandle handle) { m_PendingSceneHandle = handle; }
		AssetHandle GetPendingSceneHandle() const { return m_PendingSceneHandle; }
		void ClearPendingScene() { m_PendingSceneHandle = 0; }

		// Scene dirty state
		void MarkSceneDirty();
		bool IsSceneDirty() const { return m_SceneDirty; }

		// Callbacks
		void SetOnSceneChangedCallback(std::function<void()> callback) { m_OnSceneChanged = callback; }
		void SetOnSceneDirtyCallback(std::function<void()> callback) { m_OnSceneDirty = callback; }
		void SetOnScenePlayCallback(std::function<void()> callback) { m_OnScenePlay = callback; }
		void SetOnSceneStopCallback(std::function<void()> callback) { m_OnSceneStop = callback; }

	private:
		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		AssetHandle m_ActiveSceneHandle = 0;
		AssetHandle m_PendingSceneHandle = 0;

		SceneState m_SceneState = SceneState::Edit;
		bool m_SceneDirty = false;

		std::function<void()> m_OnSceneChanged;
		std::function<void()> m_OnSceneDirty;
		std::function<void()> m_OnScenePlay;
		std::function<void()> m_OnSceneStop;
	};

}
