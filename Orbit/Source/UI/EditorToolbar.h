#pragma once

#include "Core/RefCounted.h"
#include "Renderer/Generic/Types/Texture.h"

namespace Gravix
{
	class SceneManager;

	class EditorToolbar
	{
	public:
		EditorToolbar();
		~EditorToolbar() = default;

		void SetSceneManager(SceneManager* sceneManager) { m_SceneManager = sceneManager; }
		void OnImGuiRender();

	private:
		SceneManager* m_SceneManager = nullptr;

		Ref<Texture2D> m_IconPlay;
		Ref<Texture2D> m_IconStop;
	};

}
