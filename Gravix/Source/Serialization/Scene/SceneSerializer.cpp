#include "pch.h"
#include "SceneSerializer.h"

#include "Scene/ComponentRegistry.h"
#include "Scene/Entity.h"

namespace Gravix 
{

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{

	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetID();
		for (auto typeIndex : ComponentRegistry::Get().GetComponentOrder())
		{
			const auto& info = ComponentRegistry::Get().GetAllComponents().at(typeIndex);
			if (info.SerializeFunc && info.GetComponentFunc)
			{
				void* component = info.GetComponentFunc(m_Scene->m_Registry, entity);
				if (component)
				{
					info.SerializeFunc(out, component);
				}
			}
		}
		out << YAML::EndMap;
	}

	void SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap; 
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (auto entityID : m_Scene->m_Registry.view<TagComponent>())
		{
			Entity entity{ entityID, m_Scene.get() };
			if(!entity)
				continue;
			
			SerializeEntity(out, entity);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::filesystem::path& filepath)
	{

	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;

		YAML::Node data = YAML::Load(stream);
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		GX_CORE_TRACE("Deserializing scene: {0}", sceneName);

		uint32_t maxCreationIndex = 0;
		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				UUID uuid = (UUID)entity["Entity"].as<uint64_t>();
				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
				{
					name = tagComponent["Name"].as<std::string>();
				}
				GX_CORE_TRACE("Deserialized entity with ID: {0}, name: {1}", (uint64_t)uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntity(name, uuid);
				for (auto typeIndex : ComponentRegistry::Get().GetComponentOrder())
				{
					const auto& info = ComponentRegistry::Get().GetAllComponents().at(typeIndex);
					if (info.DeserializeFunc)
					{
						std::string componentName = info.Name + "Component";
						auto componentNode = entity[componentName];
						if (componentNode)
						{
							if (!deserializedEntity.HasComponent(typeIndex))
								deserializedEntity.AddComponent(typeIndex);

							// 2. Get pointer (now valid!)
							void* component = deserializedEntity.GetComponent(typeIndex);

							// 3. Deserialize into valid memory
							if (component)
								info.DeserializeFunc(component, componentNode);
						}
					}
				}

				// Track the maximum creation index
				auto& tag = deserializedEntity.GetComponent<TagComponent>();
				if (tag.CreationIndex > maxCreationIndex)
					maxCreationIndex = tag.CreationIndex;
			}
		}

		// Set the next creation index to be one more than the maximum
		m_Scene->m_NextCreationIndex = maxCreationIndex + 1;

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::filesystem::path& filepath)
	{
		return false;
	}

}