#pragma once

#include "Layer.h"

#include "Framebuffer.h"
#include "Material.h"
#include "Reflections/DynamicStruct.h"

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

		Ref<Gravix::Material> m_GradientMaterial;
		Ref<Gravix::Material> m_GradientColorMaterial;

		Gravix::DynamicStruct m_GradientColor;

		glm::vec2 m_ViewportSize = {1280.f, 1080.f};
	};

}