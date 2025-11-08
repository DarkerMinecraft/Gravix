#pragma once

#include "Gravix.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include <glm/glm.hpp>
#include <optional>

namespace Gravix 
{

	class AppLayer : public Layer
	{
	public:
		AppLayer();
		virtual ~AppLayer();

		virtual void OnEvent(Event& event) override;

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnRender() override;

		virtual void OnImGuiRender() override;

		void OpenScene(AssetHandle handle, bool deserialize = false);
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnFileDrop(WindowFileDropEvent& e);

		void SaveProject();
		void SaveProjectAs();
		void OpenProject();
		void NewProject();

		void SaveScene();

		void ShowStartupDialog();
		void InitializeProject();
	private:
		Ref<Framebuffer> m_MSAAFramebuffer;
		Ref<Framebuffer> m_FinalFramebuffer;

		Ref<Texture2D> m_CheckerboardTexture;
		Ref<Texture2D> m_LogoTexture;

		Ref<Scene> m_ActiveScene;

		EditorCamera m_EditorCamera;

		SceneHierarchyPanel m_SceneHierarchyPanel;
		InspectorPanel m_InspectorPanel;
		ViewportPanel m_ViewportPanel;
		std::optional<ContentBrowserPanel> m_ContentBrowserPanel;

		AssetHandle m_ActiveSceneHandle;
		AssetHandle m_PendingSceneHandle = 0; // Track scene waiting to load asynchronously
		std::filesystem::path m_ActiveProjectPath;

		bool m_ProjectInitialized = false;
		bool m_ShowStartupDialog = false;
	};

}