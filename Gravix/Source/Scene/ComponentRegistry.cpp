#include "pch.h"
#include "ComponentRegistry.h"

#include "Components.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace YAML 
{
	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& vec)
		{
			Node node;
			node.push_back(vec.x);
			node.push_back(vec.y);
			node.push_back(vec.z);
			return node;
		}
		static bool decode(const Node& node, glm::vec3& vec)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			vec.x = node[0].as<float>();
			vec.y = node[1].as<float>();
			vec.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& vec)
		{
			Node node;
			node.push_back(vec.x);
			node.push_back(vec.y);
			node.push_back(vec.z);
			node.push_back(vec.w);
			return node;
		}
		static bool decode(const Node& node, glm::vec4& vec)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			vec.x = node[0].as<float>();
			vec.y = node[1].as<float>();
			vec.z = node[2].as<float>();
			vec.w = node[3].as<float>();
			return true;
		}
	};
}

namespace Gravix
{

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& vec)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& vec)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
		return out;
	}

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
			ComponentSpecification{ .HasNodeTree = false, .CanRemoveComponent = false },
			nullptr,
			[](YAML::Emitter& out, TagComponent& c) 
			{
				out << YAML::Key << "Name" << YAML::Value << c.Name;
			},
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
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = false },
			nullptr,
			[](YAML::Emitter& out, TransformComponent& c) 
			{
				out << YAML::Key << "Position" << YAML::Value << c.Position;
				out << YAML::Key << "Rotation" << YAML::Value << c.Rotation;
				out << YAML::Key << "Scale" << YAML::Value << c.Scale;
			},
			[](TransformComponent& c, const YAML::Node& node) 
			{
				c.Position = node["Position"].as<glm::vec3>();
				c.Rotation = node["Rotation"].as<glm::vec3>();
				c.Scale = node["Scale"].as<glm::vec3>();

				c.CalculateTransform();
			},
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
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			[](CameraComponent& c, Scene* scene)
			{
				c.Camera.SetViewportSize(scene->GetViewportWidth(), scene->GetViewportHeight());
			},
			[](YAML::Emitter& out, CameraComponent& c)
			{
				auto& camera = c.Camera;
				out << YAML::Key << "Camera" << YAML::BeginMap;
				out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
				out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveFOV();
				out << YAML::Key << "PerspectiveNearClip" << YAML::Value << camera.GetPerspectiveNearClip();
				out << YAML::Key << "PerspectiveFarClip" << YAML::Value << camera.GetPerspectiveFarClip();
				out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
				out << YAML::Key << "OrthographicNearClip" << YAML::Value << camera.GetOrthographicNearClip();
				out << YAML::Key << "OrthographicFarClip" << YAML::Value << camera.GetOrthographicFarClip();
				out << YAML::EndMap;

				out << YAML::Key << "Primary" << YAML::Value << c.Primary;
				out << YAML::Key << "FixedAspectRatio" << YAML::Value << c.FixedAspectRatio;
			},
			[](CameraComponent& c, const YAML::Node& node)
			{
				auto& camera = c.Camera;
				const YAML::Node& cameraNode = node["Camera"];
				camera.SetProjectionType((ProjectionType)cameraNode["ProjectionType"].as<int>());
				camera.SetPerspectiveFOV(cameraNode["PerspectiveFOV"].as<float>());
				camera.SetPerspectiveNearClip(cameraNode["PerspectiveNearClip"].as<float>());
				camera.SetPerspectiveFarClip(cameraNode["PerspectiveFarClip"].as<float>());
				camera.SetOrthographicSize(cameraNode["OrthographicSize"].as<float>());
				camera.SetOrthographicNearClip(cameraNode["OrthographicNearClip"].as<float>());
				camera.SetOrthographicFarClip(cameraNode["OrthographicFarClip"].as<float>());
				c.Primary = node["Primary"].as<bool>();
				c.FixedAspectRatio = node["FixedAspectRatio"].as<bool>();
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
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
			[](YAML::Emitter& out, SpriteRendererComponent& c)
			{
				out << YAML::Key << "Color" << YAML::Value << c.Color;
			},
			[](SpriteRendererComponent& c, const YAML::Node& node)
			{
				c.Color = node["Color"].as<glm::vec4>();
			},
			[](SpriteRendererComponent& c)
			{
				ImGui::ColorEdit4("Color", glm::value_ptr(c.Color));
			}
		);
	}

}