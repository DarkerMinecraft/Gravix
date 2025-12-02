#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class BoxCollider2DComponentRenderer
	{
	public:
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		static void OnImGuiRender(BoxCollider2DComponent& component, ComponentUserSettings* userSettings);
		static void Serialize(YAML::Emitter& out, BoxCollider2DComponent& component);
		static void Deserialize(BoxCollider2DComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, BoxCollider2DComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, BoxCollider2DComponent& component);
	};

}
