#include "pch.h"
#include "CircleRendererComponentRenderer.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Scene/ImGuiHelpers.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#endif

namespace Gravix
{

	void CircleRendererComponentRenderer::Register()
	{
		ComponentRegistry::Get().RegisterComponent<CircleRendererComponent>(
			"Circle Renderer",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
#ifdef GRAVIX_EDITOR_BUILD
			Serialize,
			Deserialize,
			OnImGuiRender,
#endif
			BinarySerialize,
			BinaryDeserialize
		);
	}

#ifdef GRAVIX_EDITOR_BUILD
	void CircleRendererComponentRenderer::Serialize(YAML::Emitter& out, CircleRendererComponent& c)
	{
		out << YAML::Key << "Color" << YAML::Value << c.Color;
		out << YAML::Key << "Thickness" << YAML::Value << c.Thickness;
		out << YAML::Key << "Fade" << YAML::Value << c.Fade;
	}

	void CircleRendererComponentRenderer::Deserialize(CircleRendererComponent& c, const YAML::Node& node)
	{
		c.Color = node["Color"].as<glm::vec4>();
		c.Thickness = node["Thickness"].as<float>();
		c.Fade = node["Fade"].as<float>();
	}

	void CircleRendererComponentRenderer::OnImGuiRender(CircleRendererComponent& c, ComponentUserSettings*)
	{
		// Color property
		ImGuiHelpers::BeginPropertyRow("Color");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::ColorEdit4("##Color", glm::value_ptr(c.Color));
		ImGuiHelpers::EndPropertyRow();

		// Thickness property
		ImGuiHelpers::BeginPropertyRow("Thickness");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat("##Thickness", &c.Thickness, 0.01f, 0.0f, 1.0f);
		ImGuiHelpers::EndPropertyRow();

		// Fade property
		ImGuiHelpers::BeginPropertyRow("Fade");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat("##Fade", &c.Fade, 0.001f, 0.0f, 1.0f);
		ImGuiHelpers::EndPropertyRow();
	}
#endif

	void CircleRendererComponentRenderer::BinarySerialize(BinarySerializer& serializer, CircleRendererComponent& c)
	{
		serializer.Write(c.Color);
		serializer.Write(c.Thickness);
		serializer.Write(c.Fade);
	}

	void CircleRendererComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, CircleRendererComponent& c)
	{
		c.Color = deserializer.Read<glm::vec4>();
		c.Thickness = deserializer.Read<float>();
		c.Fade = deserializer.Read<float>();
	}

}
