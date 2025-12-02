#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class CircleRendererComponentRenderer
	{
	public:
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		static void OnImGuiRender(CircleRendererComponent& component, ComponentUserSettings* userSettings);
		static void Serialize(YAML::Emitter& out, CircleRendererComponent& component);
		static void Deserialize(CircleRendererComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, CircleRendererComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, CircleRendererComponent& component);
	};

}
