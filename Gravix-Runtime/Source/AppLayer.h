#pragma once

#include "Core/Gravix.h"

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
	private:
	};

}
