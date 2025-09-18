#pragma once

#include "Core/Layer.h"

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
	};

}