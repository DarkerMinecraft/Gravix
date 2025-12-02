#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class Rigidbody2DComponentRenderer
	{
	public:
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		static void OnImGuiRender(Rigidbody2DComponent& component, ComponentUserSettings* userSettings);
		static void Serialize(YAML::Emitter& out, Rigidbody2DComponent& component);
		static void Deserialize(Rigidbody2DComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, Rigidbody2DComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, Rigidbody2DComponent& component);
	};

}
