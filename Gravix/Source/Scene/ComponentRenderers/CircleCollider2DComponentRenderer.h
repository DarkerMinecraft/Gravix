#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class CircleCollider2DComponentRenderer
	{
	public:
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		static void OnImGuiRender(CircleCollider2DComponent& component, ComponentUserSettings* userSettings);
		static void Serialize(YAML::Emitter& out, CircleCollider2DComponent& component);
		static void Deserialize(CircleCollider2DComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, CircleCollider2DComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, CircleCollider2DComponent& component);
	};

}
