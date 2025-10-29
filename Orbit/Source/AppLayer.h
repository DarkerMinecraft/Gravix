#pragma once

#include "Gravix.h"

#include "Panels/SceneHierarchyPanel.h"

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
		void DrawViewportUI();
	private:
		Ref<Framebuffer> m_MainFramebuffer;

		Ref<Texture2D> m_CheckerboardTexture;
		Ref<Texture2D> m_LogoTexture;

		Ref<Scene> m_ActiveScene;

		OrthographicCamera m_Camera;
		SceneHierarchyPanel m_SceneHierarchyPanel;

		glm::vec2 m_ViewportSize = {1260, 1080};
	};

}