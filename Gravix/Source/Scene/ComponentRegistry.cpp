#include "pch.h"
#include "ComponentRegistry.h"

#include "Components.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Gravix
{

	static void DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
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

	void ComponentRegistry::RegisterAllComponents()
	{
		RegisterComponent<TagComponent>(
			"Tag",
			ComponentSettings{ .HasNodeTree = false, .CanRemoveComponent = false },
			nullptr,
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
			"Transform",
			ComponentSettings{ .HasNodeTree = true, .CanRemoveComponent = false },
			nullptr,
			[](TransformComponent& c)
			{
				// Draw the Transform component UI
				DrawVec3Control("Position", c.Position);
				DrawVec3Control("Rotation", c.Rotation);
				DrawVec3Control("Scale", c.Scale, 1.0f);
				// Update the transform matrix when values change
				c.CalculateTransform();
			}
		);

		RegisterComponent<CameraComponent>(
			"Camera",
			ComponentSettings{ .HasNodeTree = true, .CanRemoveComponent = true },
			[](CameraComponent& c, Scene* scene)
			{
				c.Camera.SetViewportSize(scene->GetViewportWidth(), scene->GetViewportHeight());
			},
			[](CameraComponent& c)
			{
				const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
				const char* currentProjectionTypeString = projectionTypeStrings[(int)c.Camera.GetProjectionType()];

				auto& camera = c.Camera;
				ImGui::Checkbox("Primary", &c.Primary);
				if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
				{
					for (int i = 0; i < 2; i++)
					{
						bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
						if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
						{
							currentProjectionTypeString = projectionTypeStrings[i];
							camera.SetProjectionType((ProjectionType)i);
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				if (camera.GetProjectionType() == ProjectionType::Orthographic)
				{
					float orthoSize = camera.GetOrthographicSize();
					if (ImGui::DragFloat("Size", &orthoSize))
						camera.SetOrthographicSize(orthoSize);

					float nearClip = camera.GetOrthographicNearClip();
					if (ImGui::DragFloat("Near Clip", &nearClip))
						camera.SetOrthographicNearClip(nearClip);

					float farClip = camera.GetOrthographicFarClip();
					if (ImGui::DragFloat("Far Clip", &farClip))
						camera.SetOrthographicFarClip(farClip);
				}

				if (camera.GetProjectionType() == ProjectionType::Perspective)
				{
					float fov = camera.GetPerspectiveFOV();
					if (ImGui::DragFloat("Vertical FOV", &fov))
						camera.SetPerspectiveFOV(fov);

					float nearClip = camera.GetPerspectiveNearClip();
					if (ImGui::DragFloat("Near Clip", &nearClip))
						camera.SetPerspectiveNearClip(nearClip);

					float farClip = camera.GetPerspectiveFarClip();
					if (ImGui::DragFloat("Far Clip", &farClip))
						camera.SetPerspectiveFarClip(farClip);
				}
				ImGui::Checkbox("Fixed Aspect Ratio", &c.FixedAspectRatio);
			}
		);

		RegisterComponent<SpriteRendererComponent>(
			"Sprite Renderer",
			ComponentSettings{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
			[](SpriteRendererComponent& c)
			{
				ImGui::ColorEdit4("Color", glm::value_ptr(c.Color));
			}
		);
	}

}