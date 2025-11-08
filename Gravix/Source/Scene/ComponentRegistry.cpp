#include "pch.h"
#include "ComponentRegistry.h"
#include "ImGuiHelpers.h"

#include "Components.h"

#include "Asset/AssetManager.h"
#include "Project/Project.h"

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

	void ComponentRegistry::RegisterAllComponents()
	{
		RegisterComponent<TagComponent>(
			"Tag",
			ComponentSpecification{ .HasNodeTree = false, .CanRemoveComponent = false },
			nullptr,
			[](YAML::Emitter& out, TagComponent& c)
			{
				out << YAML::Key << "Name" << YAML::Value << c.Name;
				out << YAML::Key << "CreationIndex" << YAML::Value << c.CreationIndex;
			},
			[](TagComponent& c, YAML::Node& node)
			{
				if (node["CreationIndex"])
					c.CreationIndex = node["CreationIndex"].as<uint32_t>();
			},
			[](TagComponent& c)
			{
				ImGuiIO& io = ImGui::GetIO();

				// Bold "Tag" label with input on the same line
				ImGui::PushFont(io.Fonts->Fonts[1]);
				ImGui::Text("Tag");
				ImGui::PopFont();
				ImGui::SameLine();

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, c.Name.c_str());
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
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
				ImGuiHelpers::DrawVec3Control("Position", c.Position);
				ImGuiHelpers::DrawVec3Control("Rotation", c.Rotation);
				ImGuiHelpers::DrawVec3Control("Scale", c.Scale, 1.0f);
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

				// Primary checkbox
				ImGuiHelpers::BeginPropertyRow("Primary");
				ImGui::Checkbox("##Primary", &c.Primary);
				ImGuiHelpers::EndPropertyRow();

				// Projection type combo
				ImGuiHelpers::BeginPropertyRow("Projection");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::BeginCombo("##Projection", currentProjectionTypeString))
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
				ImGuiHelpers::EndPropertyRow();

				if (camera.GetProjectionType() == ProjectionType::Orthographic)
				{
					ImGuiHelpers::BeginPropertyRow("Size");
					float orthoSize = camera.GetOrthographicSize();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##Size", &orthoSize))
						camera.SetOrthographicSize(orthoSize);
					ImGuiHelpers::EndPropertyRow();

					ImGuiHelpers::BeginPropertyRow("Near Clip");
					float nearClip = camera.GetOrthographicNearClip();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##NearClip", &nearClip))
						camera.SetOrthographicNearClip(nearClip);
					ImGuiHelpers::EndPropertyRow();

					ImGuiHelpers::BeginPropertyRow("Far Clip");
					float farClip = camera.GetOrthographicFarClip();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##FarClip", &farClip))
						camera.SetOrthographicFarClip(farClip);
					ImGuiHelpers::EndPropertyRow();
				}

				if (camera.GetProjectionType() == ProjectionType::Perspective)
				{
					ImGuiHelpers::BeginPropertyRow("Vertical FOV");
					float fov = camera.GetPerspectiveFOV();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##VerticalFOV", &fov))
						camera.SetPerspectiveFOV(fov);
					ImGuiHelpers::EndPropertyRow();

					ImGuiHelpers::BeginPropertyRow("Near Clip");
					float nearClip = camera.GetPerspectiveNearClip();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##NearClip", &nearClip))
						camera.SetPerspectiveNearClip(nearClip);
					ImGuiHelpers::EndPropertyRow();

					ImGuiHelpers::BeginPropertyRow("Far Clip");
					float farClip = camera.GetPerspectiveFarClip();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##FarClip", &farClip))
						camera.SetPerspectiveFarClip(farClip);
					ImGuiHelpers::EndPropertyRow();
				}

				ImGuiHelpers::BeginPropertyRow("Fixed Aspect");
				ImGui::Checkbox("##FixedAspectRatio", &c.FixedAspectRatio);
				ImGuiHelpers::EndPropertyRow();
			}
		);

		RegisterComponent<SpriteRendererComponent>(
			"Sprite Renderer",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
			[](YAML::Emitter& out, SpriteRendererComponent& c)
			{
				out << YAML::Key << "Color" << YAML::Value << c.Color;
				out << YAML::Key << "Texture" << YAML::Value << (uint64_t)c.Texture;
				out << YAML::Key << "TilingFactor" << YAML::Value << c.TilingFactor;
			},
			[](SpriteRendererComponent& c, const YAML::Node& node)
			{
				c.Color = node["Color"].as<glm::vec4>();
				c.Texture = (AssetHandle)node["Texture"].as<uint64_t>();
				c.TilingFactor = node["TilingFactor"].as<float>();
			},
			[](SpriteRendererComponent& c)
			{
				// Color property
				ImGuiHelpers::BeginPropertyRow("Color");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::ColorEdit4("##Color", glm::value_ptr(c.Color));
				ImGuiHelpers::EndPropertyRow();

				// Texture property
				ImGuiHelpers::BeginPropertyRow("Texture");

				std::string label = "None";
				bool validTexture = false;
				if (c.Texture != 0)
				{
					if (AssetManager::IsValidAssetHandle(c.Texture) && AssetManager::GetAssetType(c.Texture) == AssetType::Texture2D)
					{
						const auto& metadata = Project::GetActive()->GetEditorAssetManager()->GetAssetMetadata(c.Texture);
						label = metadata.FilePath.filename().string();
						validTexture = true;
					}
					else
					{
						label = "Invalid";
					}
				}

				float availWidth = ImGui::GetContentRegionAvail().x;
				float buttonWidth = validTexture ? availWidth - 30.0f : availWidth;

				ImGui::Button(label.c_str(), { buttonWidth, 0.0f });
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						AssetHandle textureHandle = *(AssetHandle*)payload->Data;
						if(AssetManager::GetAssetType(textureHandle) == AssetType::Texture2D)
							c.Texture = textureHandle;
					}
					ImGui::EndDragDropTarget();
				}

				if (validTexture)
				{
					ImGui::SameLine();
					if (ImGui::Button("X", { 26.0f, 0.0f }))
						c.Texture = 0;
				}

				ImGuiHelpers::EndPropertyRow();

				// Tiling Factor property
				ImGuiHelpers::BeginPropertyRow("Tiling Factor");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##TilingFactor", &c.TilingFactor);
				ImGuiHelpers::EndPropertyRow();
			}
		);
	}

}