#pragma once

#include "Events/Event.h"

namespace Gravix 
{
	class ImGuiRender 
	{
	public:
		ImGuiRender();
		~ImGuiRender();

		void Begin();
		void End();

		void OnEvent(Event& e);
	private:
		void Init();
		void SetTheme();
	private:
		bool m_BlockEvents = false;
	};
}