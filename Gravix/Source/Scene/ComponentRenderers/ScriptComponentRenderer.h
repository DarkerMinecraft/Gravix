#pragma once

#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

namespace Gravix
{

	// Handles all ImGui rendering and serialization for ScriptComponent
	class ScriptComponentRenderer
	{
	public:
		// Register the ScriptComponent with the ComponentRegistry
		static void Register();

	private:
#ifdef GRAVIX_EDITOR_BUILD
		// ImGui rendering callback
		static void OnImGuiRender(ScriptComponent& component, ComponentUserSettings* userSettings);

		// YAML serialization callback
		static void Serialize(YAML::Emitter& out, ScriptComponent& component);

		// YAML deserialization callback
		static void Deserialize(ScriptComponent& component, const YAML::Node& node);

		// Helper: Convert camelCase to Title Case with spaces
		static std::string CamelCaseToTitleCase(const std::string& input);
#endif

		// Binary serialization
		static void BinarySerialize(BinarySerializer& serializer, ScriptComponent& component);
		static void BinaryDeserialize(BinaryDeserializer& deserializer, ScriptComponent& component);
	};

}
