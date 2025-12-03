#include "pch.h"
#include "ConsolePanel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <iomanip>
#include <sstream>

namespace Gravix
{

	void ConsolePanel::OnImGuiRender()
	{
		ImGui::Begin("Console");

		// Toolbar
		{
			// Clear button
			if (ImGui::Button("Clear"))
			{
				Console::Clear();
			}

			ImGui::SameLine();

			// Filter buttons with counts
			uint32_t logCount = Console::GetLogCount();
			uint32_t warningCount = Console::GetWarningCount();
			uint32_t errorCount = Console::GetErrorCount();

			// Log filter
			ImGui::PushStyleColor(ImGuiCol_Button, m_ShowLogs ? ImVec4(0.2f, 0.6f, 1.0f, 0.6f) : ImVec4(0.2f, 0.2f, 0.2f, 0.4f));
			if (ImGui::Button(("Logs: " + std::to_string(logCount)).c_str()))
			{
				m_ShowLogs = !m_ShowLogs;
			}
			ImGui::PopStyleColor();

			ImGui::SameLine();

			// Warning filter
			ImGui::PushStyleColor(ImGuiCol_Button, m_ShowWarnings ? ImVec4(1.0f, 0.8f, 0.2f, 0.6f) : ImVec4(0.2f, 0.2f, 0.2f, 0.4f));
			if (ImGui::Button(("Warnings: " + std::to_string(warningCount)).c_str()))
			{
				m_ShowWarnings = !m_ShowWarnings;
			}
			ImGui::PopStyleColor();

			ImGui::SameLine();

			// Error filter
			ImGui::PushStyleColor(ImGuiCol_Button, m_ShowErrors ? ImVec4(1.0f, 0.3f, 0.3f, 0.6f) : ImVec4(0.2f, 0.2f, 0.2f, 0.4f));
			if (ImGui::Button(("Errors: " + std::to_string(errorCount)).c_str()))
			{
				m_ShowErrors = !m_ShowErrors;
			}
			ImGui::PopStyleColor();

			ImGui::SameLine();

			// Collapse toggle
			ImGui::Checkbox("Collapse", &m_CollapseMessages);

			ImGui::SameLine();

			// Auto-scroll toggle
			ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

			// Search bar
			ImGui::SameLine();
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer));
		}

		ImGui::Separator();

		// Message list
		{
			ImGui::BeginChild("MessageList", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			const auto& messages = Console::GetMessages();
			std::string searchFilter = m_SearchBuffer;

			for (const auto& message : messages)
			{
				// Filter by type
				if (message.Type == ConsoleMessageType::Log && !m_ShowLogs)
					continue;
				if (message.Type == ConsoleMessageType::Warning && !m_ShowWarnings)
					continue;
				if (message.Type == ConsoleMessageType::Error && !m_ShowErrors)
					continue;

				// Filter by search
				if (!searchFilter.empty())
				{
					if (message.Message.find(searchFilter) == std::string::npos)
						continue;
				}

				DrawMessage(message);
			}

			// Auto-scroll to bottom
			if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			{
				ImGui::SetScrollHereY(1.0f);
			}

			ImGui::EndChild();
		}

		ImGui::End();
	}

	void ConsolePanel::DrawMessage(const ConsoleMessage& message)
	{
		ImGui::PushID(&message);

		// Message color
		glm::vec4 color = GetMessageColor(message.Type);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.x, color.y, color.z, color.w));

		// Icon
		const char* icon = GetMessageIcon(message.Type);

		// Format timestamp
		auto timeT = std::chrono::system_clock::to_time_t(message.Timestamp);
		std::tm tm;
		localtime_s(&tm, &timeT);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%H:%M:%S");
		std::string timestamp = oss.str();

		// Build message text
		std::string displayText = "[" + timestamp + "] " + icon + " " + message.Message;

		// Add count if collapsed
		if (m_CollapseMessages && message.Count > 1)
		{
			displayText += " (" + std::to_string(message.Count) + ")";
		}

		// Selectable for copy functionality
		if (ImGui::Selectable(displayText.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if (ImGui::IsMouseDoubleClicked(0))
			{
				ImGui::SetClipboardText(message.Message.c_str());
			}
		}

		// Tooltip with full message on hover
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(message.Message.c_str());
			ImGui::Text("Double-click to copy");
			ImGui::EndTooltip();
		}

		ImGui::PopStyleColor();
		ImGui::PopID();
	}

	const char* ConsolePanel::GetMessageIcon(ConsoleMessageType type) const
	{
		switch (type)
		{
		case ConsoleMessageType::Log:     return "[INFO]";
		case ConsoleMessageType::Warning: return "[WARN]";
		case ConsoleMessageType::Error:   return "[ERROR]";
		}
		return "[???]";
	}

	glm::vec4 ConsolePanel::GetMessageColor(ConsoleMessageType type) const
	{
		switch (type)
		{
		case ConsoleMessageType::Log:     return glm::vec4{ 0.8f, 0.8f, 0.8f, 1.0f };  // Light gray
		case ConsoleMessageType::Warning: return glm::vec4{ 1.0f, 0.8f, 0.2f, 1.0f };  // Yellow
		case ConsoleMessageType::Error:   return glm::vec4{ 1.0f, 0.3f, 0.3f, 1.0f };  // Red
		}
		return glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}

}
