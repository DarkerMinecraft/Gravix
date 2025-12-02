#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class ComponentOrderComponentRenderer
	{
	public:
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		static void Serialize(YAML::Emitter& out, ComponentOrderComponent& component);
		static void Deserialize(ComponentOrderComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, ComponentOrderComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, ComponentOrderComponent& component);
	};

}
