#include "pch.h"
#include "ImGuiHelpers.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui_internal.h>

namespace Gravix
{

	void ImGuiHelpers::BeginPropertyRow(const char* label, float columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::PushID(label);
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);

		// Bold label on the left
		ImGui::PushFont(io.Fonts->Fonts[1]);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label);
		ImGui::PopFont();

		// Draw vertical separator line
		ImVec2 lineStart = ImGui::GetCursorScreenPos();
		lineStart.x += columnWidth - 1.0f;
		lineStart.y -= ImGui::GetTextLineHeightWithSpacing();
		ImVec2 lineEnd = lineStart;
		lineEnd.y += ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2.0f;
		ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);

		ImGui::NextColumn();
	}

	void ImGuiHelpers::EndPropertyRow()
	{
		ImGui::Columns(1);
		ImGui::PopID();
	}

	void ImGuiHelpers::DrawVec3Control(const char* label, glm::vec3& values, float resetValue, float columnWidth)
	{
		BeginPropertyRow(label, columnWidth);

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 2, 0 });  // Slight spacing between controls

		float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		// X Axis (Red) - Unity-style vibrant red
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.7f, 0.1f, 0.1f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.85f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.95f, 0.3f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopStyleColor(4);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Y Axis (Green) - Unity-style vibrant green
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.15f, 0.65f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.25f, 0.75f, 0.25f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.35f, 0.85f, 0.35f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopStyleColor(4);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Z Axis (Blue) - Unity-style vibrant blue
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.3f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.4f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.3f, 0.5f, 1.0f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopStyleColor(4);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		EndPropertyRow();
	}

}
