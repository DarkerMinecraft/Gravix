#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class TagComponentRenderer
	{
	public:
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		static void OnImGuiRender(TagComponent& component, ComponentUserSettings* userSettings);
		static void Serialize(YAML::Emitter& out, TagComponent& component);
		static void Deserialize(TagComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, TagComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, TagComponent& component);
	};

}
