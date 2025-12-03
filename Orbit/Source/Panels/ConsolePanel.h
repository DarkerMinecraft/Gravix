#pragma once

#include "Core/Console.h"

#include <glm/glm.hpp>

namespace Gravix
{

	class ConsolePanel
	{
	public:
		ConsolePanel() = default;
		~ConsolePanel() = default;

		void OnImGuiRender();

	private:
		void DrawMessage(const ConsoleMessage& message);
		const char* GetMessageIcon(ConsoleMessageType type) const;
		glm::vec4 GetMessageColor(ConsoleMessageType type) const;

	private:
		// Filter flags
		bool m_ShowLogs = true;
		bool m_ShowWarnings = true;
		bool m_ShowErrors = true;

		// Collapse identical messages
		bool m_CollapseMessages = true;

		// Auto-scroll to bottom
		bool m_AutoScroll = true;

		// Search filter
		char m_SearchBuffer[256] = "";
	};

}
