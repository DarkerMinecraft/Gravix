#pragma once

#include "Events/Event.h"

namespace Gravix 
{

	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnEvent(Event& event) {}

		virtual void OnUpdate(float deltaTime) {}
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
	};

}