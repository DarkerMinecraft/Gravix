#include "pch.h"
#include "CameraComponentRenderer.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Scene/ImGuiHelpers.h"
#include <imgui.h>
#endif

namespace Gravix
{

	void CameraComponentRenderer::Register()
	{
		ComponentRegistry::Get().RegisterComponent<CameraComponent>(
			"Camera",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			OnComponentAdded,
#ifdef GRAVIX_EDITOR_BUILD
			Serialize,
			Deserialize,
			OnImGuiRender,
#endif
			BinarySerialize,
			BinaryDeserialize
		);
	}

	void CameraComponentRenderer::OnComponentAdded(CameraComponent& c, Scene* scene)
	{
		c.Camera.SetViewportSize(scene->GetViewportWidth(), scene->GetViewportHeight());
	}

#ifdef GRAVIX_EDITOR_BUILD

	void CameraComponentRenderer::Serialize(YAML::Emitter& out, CameraComponent& c)
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
	}

	void CameraComponentRenderer::Deserialize(CameraComponent& c, const YAML::Node& node)
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
	}

	void CameraComponentRenderer::OnImGuiRender(CameraComponent& c, ComponentUserSettings*)
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
#endif

	void CameraComponentRenderer::BinarySerialize(BinarySerializer& serializer, CameraComponent& c)
	{
		auto& camera = c.Camera;
		serializer.Write(static_cast<int>(camera.GetProjectionType()));
		serializer.Write(camera.GetPerspectiveFOV());
		serializer.Write(camera.GetPerspectiveNearClip());
		serializer.Write(camera.GetPerspectiveFarClip());
		serializer.Write(camera.GetOrthographicSize());
		serializer.Write(camera.GetOrthographicNearClip());
		serializer.Write(camera.GetOrthographicFarClip());
		serializer.Write(c.Primary);
		serializer.Write(c.FixedAspectRatio);
	}

	void CameraComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, CameraComponent& c)
	{
		auto& camera = c.Camera;
		camera.SetProjectionType(static_cast<ProjectionType>(deserializer.Read<int>()));
		camera.SetPerspectiveFOV(deserializer.Read<float>());
		camera.SetPerspectiveNearClip(deserializer.Read<float>());
		camera.SetPerspectiveFarClip(deserializer.Read<float>());
		camera.SetOrthographicSize(deserializer.Read<float>());
		camera.SetOrthographicNearClip(deserializer.Read<float>());
		camera.SetOrthographicFarClip(deserializer.Read<float>());
		c.Primary = deserializer.Read<bool>();
		c.FixedAspectRatio = deserializer.Read<bool>();
	}

}
