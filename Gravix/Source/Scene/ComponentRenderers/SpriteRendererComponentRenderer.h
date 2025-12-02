#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class SpriteRendererComponentRenderer
	{
	public:
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		static void OnImGuiRender(SpriteRendererComponent& component, ComponentUserSettings* userSettings);
		static void Serialize(YAML::Emitter& out, SpriteRendererComponent& component);
		static void Deserialize(SpriteRendererComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, SpriteRendererComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, SpriteRendererComponent& component);
	};

}
