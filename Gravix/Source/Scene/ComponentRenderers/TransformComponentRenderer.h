#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class TransformComponentRenderer
	{
	public:
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		static void OnImGuiRender(TransformComponent& component, ComponentUserSettings* userSettings);
		static void Serialize(YAML::Emitter& out, TransformComponent& component);
		static void Deserialize(TransformComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, TransformComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, TransformComponent& component);
	};

}
