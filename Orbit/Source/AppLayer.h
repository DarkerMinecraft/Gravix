#pragma once

#include "Gravix.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ViewportPanel.h"

#include <glm/glm.hpp>

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

		std::filesystem::path m_ActiveScenePath;
	};

}