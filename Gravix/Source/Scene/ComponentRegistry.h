#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <entt/entt.hpp>

namespace Gravix
{

	class Scene;

	struct ComponentInfo
	{
		std::string Name;
		std::function<void(void*, Scene* scene)> OnCreateFunc;
		//std::function<void* (YAML::Emitter&, void*) SerializeFunc;
		//std::function<void*(void*, const YAML::Node&>) DeserializeFunc;
		std::function<void(void*)> ImGuiRenderFunc;
		// Add a function to retrieve the component from a registry at runtime
		std::function<void* (entt::registry&, entt::entity)> GetComponentFunc;
	};

	class ComponentRegistry
	{
	public:
		template<typename T>
		void RegisterComponent(const std::string& name,
			std::function<void(T&, Scene* scene)> onCreate,
			/*std::function<void(YAML::Emitter&, T&)> serialize,
			std::function<void(T&, const YAML::Node&)> deserialize,*/
			std::function<void(T&)> imguiRender)
		{
			ComponentInfo info;
			info.Name = name;
			info.OnCreateFunc = [onCreate](void* instance, Scene* scene)
				{
					if(onCreate)
						onCreate(*reinterpret_cast<T*>(instance), scene);
				};
			/*
			info.SerializeFunc = [serialize](YAML::Emitter& out, void* instance)
				{
					serialize(out, *reinterpret_cast<T*>(instance));
				};
			info.DeserializeFunc = [deserialize](void* instance, const YAML::Node& node)
				{
					deserialize(*reinterpret_cast<T*>(instance), node);
				};
			*/
			info.ImGuiRenderFunc = [imguiRender](void* instance)
				{
					imguiRender(*reinterpret_cast<T*>(instance));
				};

			// Add the getter function to retrieve component at runtime
			info.GetComponentFunc = [](entt::registry& registry, entt::entity entity) -> void*
				{
					if (registry.all_of<T>(entity))
					{
						return &registry.get<T>(entity);
					}
					return nullptr;
				};

			m_Components[typeid(T)] = info;
		}

		const ComponentInfo* GetComponentInfo(std::type_index type) const
		{
			auto it = m_Components.find(type);
			if (it != m_Components.end())
				return &it->second;
			return nullptr;
		}

		void RegisterAllComponents();

		static ComponentRegistry& Get()
		{
			static ComponentRegistry instance;
			return instance;
		}
	public:
		const std::unordered_map<std::type_index, ComponentInfo>& GetAllComponents() const { return m_Components; }
	private:
		std::unordered_map<std::type_index, ComponentInfo> m_Components;
	};

}