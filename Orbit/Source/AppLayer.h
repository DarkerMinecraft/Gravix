#pragma once

#include "Gravix.h"

#include <glm/glm.hpp>

namespace Orbit 
{

	class AppLayer : public Gravix::Layer
	{
	public:
		AppLayer();
		virtual ~AppLayer();

		virtual void OnEvent(Gravix::Event& event) override;

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnRender() override;

		virtual void OnImGuiRender() override;
	private:
		void DrawViewportUI();
	private:
		Ref<Gravix::Framebuffer> m_MainFramebuffer;

		Ref<Gravix::Texture2D> m_CheckerboardTexture;
		Ref<Gravix::Texture2D> m_LogoTexture;

		Ref<Gravix::Scene> m_ActiveScene;

		Gravix::OrthographicCamera m_Camera;

		glm::vec2 m_ViewportSize = {1260, 1080};
	};

}