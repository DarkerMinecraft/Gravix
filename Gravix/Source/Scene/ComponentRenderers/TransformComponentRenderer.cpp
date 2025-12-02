#include "pch.h"
#include "TransformComponentRenderer.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Scene/ImGuiHelpers.h"
#endif

#include <glm/glm.hpp>

namespace Gravix
{

	void TransformComponentRenderer::Register()
	{
		ComponentRegistry::Get().RegisterComponent<TransformComponent>(
			"Transform",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = false },
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
	void TransformComponentRenderer::Serialize(YAML::Emitter& out, TransformComponent& c)
	{
		out << YAML::Key << "Position" << YAML::Value << c.Position;
		out << YAML::Key << "Rotation" << YAML::Value << c.Rotation;
		out << YAML::Key << "Scale" << YAML::Value << c.Scale;
	}

	void TransformComponentRenderer::Deserialize(TransformComponent& c, const YAML::Node& node)
	{
		c.Position = node["Position"].as<glm::vec3>();
		c.Rotation = node["Rotation"].as<glm::vec3>();
		c.Scale = node["Scale"].as<glm::vec3>();

		c.CalculateTransform();
	}

	void TransformComponentRenderer::OnImGuiRender(TransformComponent& c, ComponentUserSettings*)
	{
		// Draw the Transform component UI
		ImGuiHelpers::DrawVec3Control("Position", c.Position);
		ImGuiHelpers::DrawVec3Control("Rotation", c.Rotation);
		ImGuiHelpers::DrawVec3Control("Scale", c.Scale, 1.0f);
		// Update the transform matrix when values change
		c.CalculateTransform();
	}
#endif

	void TransformComponentRenderer::BinarySerialize(BinarySerializer& serializer, TransformComponent& c)
	{
		serializer.Write(c.Position);
		serializer.Write(c.Rotation);
		serializer.Write(c.Scale);
	}

	void TransformComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, TransformComponent& c)
	{
		c.Position = deserializer.Read<glm::vec3>();
		c.Rotation = deserializer.Read<glm::vec3>();
		c.Scale = deserializer.Read<glm::vec3>();

		c.CalculateTransform();
	}

}
