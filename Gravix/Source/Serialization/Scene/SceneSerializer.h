#pragma once

#include "Scene/Scene.h"

#include <filesystem>
#include <yaml-cpp/yaml.h>	

namespace Gravix 
{
	
	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		void SerializeEntity(YAML::Emitter& out, Entity entity);

		void Serialize(const std::filesystem::path& filepath);
		void SerializeRuntime(const std::filesystem::path& filepath);

		bool Deserialize(const std::filesystem::path& filepath);
		bool DeserializeRuntime(const std::filesystem::path& filepath);
	private:
		Ref<Scene> m_Scene;
	};

} 
