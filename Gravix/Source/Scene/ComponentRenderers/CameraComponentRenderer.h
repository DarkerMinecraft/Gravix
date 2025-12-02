#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	class CameraComponentRenderer
	{
	public:
		static void Register();

	private:
		static void OnComponentAdded(CameraComponent& component, Scene* scene);
#ifdef GRAVIX_EDITOR_BUILD
		static void OnImGuiRender(CameraComponent& component, ComponentUserSettings* userSettings);
		static void Serialize(YAML::Emitter& out, CameraComponent& component);
		static void Deserialize(CameraComponent& component, const YAML::Node& node);
#endif
		static void BinarySerialize(BinarySerializer& serializer, CameraComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, CameraComponent& component);
	};

}
