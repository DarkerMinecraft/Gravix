#include "pch.h"
#include "ComponentRegistry.h"

#include "Components.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Gravix 
{

	void ComponentRegistry::RegisterAllComponents()
	{
		RegisterComponent<TagComponent>(
			"TagComponent",
			[](TagComponent& c, Scene* scene)
			{
			},
			[](TagComponent& c)
			{
				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, c.Name.c_str());
				if (ImGui::InputText("##TagComponentName", buffer, sizeof(buffer)))
				{
					c.Name = std::string(buffer);
				}
			}
		);

		RegisterComponent<TransformComponent>(
			"TransformComponent",
			nullptr,
			[](TransformComponent& c)
			{
				auto DrawVec3Control = [](const char* label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
					{
						ImGuiIO& io = ImGui::GetIO();

						ImGui::PushID(label);

						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, columnWidth);
						ImGui::Text(label);
						ImGui::NextColumn();

						ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

						float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
						ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

						// X Axis (Red)
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
						if (ImGui::Button("X", buttonSize))
							values.x = resetValue;
						ImGui::PopStyleColor(3);

						ImGui::SameLine();
						ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
						ImGui::PopItemWidth();
						ImGui::SameLine();

						// Y Axis (Green)
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
						if (ImGui::Button("Y", buttonSize))
							values.y = resetValue;
						ImGui::PopStyleColor(3);

						ImGui::SameLine();
						ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
						ImGui::PopItemWidth();
						ImGui::SameLine();

						// Z Axis (Blue)
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
						if (ImGui::Button("Z", buttonSize))
							values.z = resetValue;
						ImGui::PopStyleColor(3);

						ImGui::SameLine();
						ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
						ImGui::PopItemWidth();

						ImGui::PopStyleVar();

						ImGui::Columns(1);

						ImGui::PopID();
					};

				// Draw the Transform component UI
				DrawVec3Control("Position", c.Position);
				DrawVec3Control("Rotation", c.Rotation);
				DrawVec3Control("Scale", c.Scale, 1.0f);

				// Update the transform matrix when values change
				c.Transform = glm::translate(glm::mat4(1.0f), c.Position)
					* glm::rotate(glm::mat4(1.0f), glm::radians(c.Rotation.x), { 1.0f, 0.0f, 0.0f })
					* glm::rotate(glm::mat4(1.0f), glm::radians(c.Rotation.y), { 0.0f, 1.0f, 0.0f })
					* glm::rotate(glm::mat4(1.0f), glm::radians(c.Rotation.z), { 0.0f, 0.0f, 1.0f })
					* glm::scale(glm::mat4(1.0f), c.Scale);
			}
		);

		RegisterComponent<CameraComponent>(
			"CameraCompoent",
			[](CameraComponent& c, Scene* scene) 
			{

			},
			[](CameraComponent& c) 
			{

			}
		);
	}

}