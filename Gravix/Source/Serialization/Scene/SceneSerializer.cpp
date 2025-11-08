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

		// Collect all entities
		std::vector<Entity> entities;
		for (auto entityID : m_Scene->m_Registry.view<TagComponent>())
		{
			Entity entity{ entityID, m_Scene.get() };
			if(entity)
				entities.push_back(entity);
		}

		// Sort entities by CreationIndex to preserve creation order
		std::sort(entities.begin(), entities.end(),
			[](const Entity& a, const Entity& b)
			{
				return a.GetComponent<TagComponent>().CreationIndex < b.GetComponent<TagComponent>().CreationIndex;
			});

		// Serialize entities in sorted order
		for (const auto& entity : entities)
		{
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
				uint32_t creationIndex = 0;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
				{
					name = tagComponent["Name"].as<std::string>();
					creationIndex = tagComponent["CreationIndex"].as<uint32_t>();
				}
				GX_CORE_TRACE("Deserialized entity with ID: {0}, name: {1}", (uint64_t)uuid, name);

				// Track max creation index to update Scene's counter
				if (creationIndex > maxCreationIndex)
					maxCreationIndex = creationIndex;

				Entity deserializedEntity = m_Scene->CreateEntity(name, uuid, creationIndex);

				// First pass: Deserialize ComponentOrderComponent if it exists
				std::vector<std::type_index> componentOrder;
				auto componentOrderNode = entity["ComponentOrderComponent"];
				if (componentOrderNode && deserializedEntity.HasComponent<ComponentOrderComponent>())
				{
					auto& orderComponent = deserializedEntity.GetComponent<ComponentOrderComponent>();
					const auto& info = ComponentRegistry::Get().GetAllComponents().at(typeid(ComponentOrderComponent));
					if (info.DeserializeFunc)
					{
						info.DeserializeFunc(&orderComponent, componentOrderNode);
						componentOrder = orderComponent.ComponentOrder;
					}
				}

				// If we have a saved component order, use it; otherwise use registry order
				if (componentOrder.empty())
				{
					componentOrder = ComponentRegistry::Get().GetComponentOrder();
				}

				// Second pass: Deserialize components in the correct order
				for (auto typeIndex : componentOrder)
				{
					// Skip ComponentOrderComponent as it's already handled
					if (typeIndex == typeid(ComponentOrderComponent))
						continue;

					// Skip TagComponent and TransformComponent as they're created automatically
					if (typeIndex == typeid(TagComponent) || typeIndex == typeid(TransformComponent))
						continue;

					const auto& allComponents = ComponentRegistry::Get().GetAllComponents();
					auto it = allComponents.find(typeIndex);
					if (it == allComponents.end())
						continue;

					const auto& info = it->second;
					if (info.DeserializeFunc)
					{
						std::string componentName = info.Name + "Component";
						auto componentNode = entity[componentName];
						if (componentNode)
						{
							if (!deserializedEntity.HasComponent(typeIndex))
								deserializedEntity.AddComponent(typeIndex);

							void* component = deserializedEntity.GetComponent(typeIndex);
							if (component)
								info.DeserializeFunc(component, componentNode);
						}
					}
				}

				// Third pass: Deserialize TagComponent and TransformComponent data (they already exist)
				auto tagNode = entity["TagComponent"];
				if (tagNode)
				{
					auto& tc = deserializedEntity.GetComponent<TagComponent>();
					const auto& info = ComponentRegistry::Get().GetAllComponents().at(typeid(TagComponent));
					if (info.DeserializeFunc)
						info.DeserializeFunc(&tc, tagNode);
				}

				auto transformNode = entity["TransformComponent"];
				if (transformNode)
				{
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					const auto& info = ComponentRegistry::Get().GetAllComponents().at(typeid(TransformComponent));
					if (info.DeserializeFunc)
						info.DeserializeFunc(&tc, transformNode);
				}

				// Restore the component order to remove any duplicates added during deserialization
				if (!componentOrder.empty() && deserializedEntity.HasComponent<ComponentOrderComponent>())
				{
					auto& orderComponent = deserializedEntity.GetComponent<ComponentOrderComponent>();
					orderComponent.ComponentOrder = componentOrder;
				}
			}
		}

		// Update Scene's next creation index to be higher than any loaded entity
		m_Scene->m_NextCreationIndex = maxCreationIndex + 1;

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::filesystem::path& filepath)
	{
		return false;
	}

}