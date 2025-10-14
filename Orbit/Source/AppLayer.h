#pragma once

#include "Layer.h"

#include "Framebuffer.h"
#include "Material.h"
#include "Texture.h"
#include "Reflections/DynamicStruct.h"
#include "OrthographicCamera.h"

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

		Gravix::OrthographicCamera m_Camera;

		glm::vec2 m_ViewportSize = {1260, 1080};
	};

}