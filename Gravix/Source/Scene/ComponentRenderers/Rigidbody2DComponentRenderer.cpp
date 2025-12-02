#include "pch.h"
#include "Rigidbody2DComponentRenderer.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Scene/ImGuiHelpers.h"
#include <imgui.h>
#endif

namespace Gravix
{

	void Rigidbody2DComponentRenderer::Register()
	{
		ComponentRegistry::Get().RegisterComponent<Rigidbody2DComponent>(
			"Rigidbody2D",
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
	void Rigidbody2DComponentRenderer::Serialize(YAML::Emitter& out, Rigidbody2DComponent& c)
	{
		out << YAML::Key << "BodyType" << YAML::Value << (int)c.Type;
		out << YAML::Key << "FixedRotation" << YAML::Value << c.FixedRotation;
	}

	void Rigidbody2DComponentRenderer::Deserialize(Rigidbody2DComponent& c, const YAML::Node& node)
	{
		c.Type = (Rigidbody2DComponent::BodyType)node["BodyType"].as<int>();
		c.FixedRotation = node["FixedRotation"].as<bool>();
	}

	void Rigidbody2DComponentRenderer::OnImGuiRender(Rigidbody2DComponent& c, ComponentUserSettings*)
	{
		const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
		const char* currentBodyTypeString = bodyTypeStrings[(int)c.Type];

		// Body Type combo
		ImGuiHelpers::BeginPropertyRow("Body Type");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##BodyType", currentBodyTypeString))
		{
			for (int i = 0; i < 3; i++)
			{
				bool isSelected = (int)c.Type == i;
				if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
				{
					c.Type = (Rigidbody2DComponent::BodyType)i;
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
		ImGuiHelpers::EndPropertyRow();

		// Fixed Rotation checkbox
		ImGuiHelpers::BeginPropertyRow("Fixed Rotation");
		ImGui::Checkbox("##FixedRotation", &c.FixedRotation);
		ImGuiHelpers::EndPropertyRow();
	}
#endif

	void Rigidbody2DComponentRenderer::BinarySerialize(BinarySerializer& serializer, Rigidbody2DComponent& c)
	{
		serializer.Write(static_cast<int>(c.Type));
		serializer.Write(c.FixedRotation);
	}

	void Rigidbody2DComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, Rigidbody2DComponent& c)
	{
		c.Type = static_cast<Rigidbody2DComponent::BodyType>(deserializer.Read<int>());
		c.FixedRotation = deserializer.Read<bool>();
	}

}
