#pragma once

#include "Scene/Scene.h"
#include "Serialization/BinarySerializer.h"
#include "Serialization/BinaryDeserializer.h"

#include <vector>

#ifdef GRAVIX_EDITOR_BUILD
#include <filesystem>
#include <yaml-cpp/yaml.h>
#endif

namespace Gravix
{

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

#ifdef GRAVIX_EDITOR_BUILD
		void SerializeEntity(YAML::Emitter& out, Entity entity);
		void Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath, YAML::Node* outNode);
#endif

		// Runtime: Binary serialization
		void SerializeRuntime(BinarySerializer& serializer);
		bool DeserializeRuntime(BinaryDeserializer& deserializer);

	private:
		Ref<Scene> m_Scene;
	};

} 
