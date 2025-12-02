#include "pch.h"
#include "BoxCollider2DComponentRenderer.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Scene/ImGuiHelpers.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#endif

namespace Gravix
{

	void BoxCollider2DComponentRenderer::Register()
	{
		ComponentRegistry::Get().RegisterComponent<BoxCollider2DComponent>(
			"BoxCollider2D",
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
	void BoxCollider2DComponentRenderer::Serialize(YAML::Emitter& out, BoxCollider2DComponent& c)
	{
		out << YAML::Key << "Offset" << YAML::Value << c.Offset;
		out << YAML::Key << "Size" << YAML::Value << c.Size;
		out << YAML::Key << "Density" << YAML::Value << c.Density;
		out << YAML::Key << "Friction" << YAML::Value << c.Friction;
		out << YAML::Key << "Restitution" << YAML::Value << c.Restitution;
	}

	void BoxCollider2DComponentRenderer::Deserialize(BoxCollider2DComponent& c, const YAML::Node& node)
	{
		c.Offset = node["Offset"].as<glm::vec2>();
		c.Size = node["Size"].as<glm::vec2>();
		c.Density = node["Density"].as<float>();
		c.Friction = node["Friction"].as<float>();
		c.Restitution = node["Restitution"].as<float>();
	}

	void BoxCollider2DComponentRenderer::OnImGuiRender(BoxCollider2DComponent& c, ComponentUserSettings*)
	{
		// Offset property
		ImGuiHelpers::BeginPropertyRow("Offset");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat2("##Offset", glm::value_ptr(c.Offset), 0.01f);
		ImGuiHelpers::EndPropertyRow();

		// Size property
		ImGuiHelpers::BeginPropertyRow("Size");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat2("##Size", glm::value_ptr(c.Size), 0.01f);
		ImGuiHelpers::EndPropertyRow();

		// Density property
		ImGuiHelpers::BeginPropertyRow("Density");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat("##Density", &c.Density, 0.01f, 0.0f, 100.0f);
		ImGuiHelpers::EndPropertyRow();

		// Friction property
		ImGuiHelpers::BeginPropertyRow("Friction");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat("##Friction", &c.Friction, 0.01f, 0.0f, 1.0f);
		ImGuiHelpers::EndPropertyRow();

		// Restitution property
		ImGuiHelpers::BeginPropertyRow("Restitution");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat("##Restitution", &c.Restitution, 0.01f, 0.0f, 1.0f);
		ImGuiHelpers::EndPropertyRow();
	}
#endif

	void BoxCollider2DComponentRenderer::BinarySerialize(BinarySerializer& serializer, BoxCollider2DComponent& c)
	{
		serializer.Write(c.Offset);
		serializer.Write(c.Size);
		serializer.Write(c.Density);
		serializer.Write(c.Friction);
		serializer.Write(c.Restitution);
	}

	void BoxCollider2DComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, BoxCollider2DComponent& c)
	{
		c.Offset = deserializer.Read<glm::vec2>();
		c.Size = deserializer.Read<glm::vec2>();
		c.Density = deserializer.Read<float>();
		c.Friction = deserializer.Read<float>();
		c.Restitution = deserializer.Read<float>();
	}

}
