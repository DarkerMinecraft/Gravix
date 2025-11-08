#pragma once

#include "Scene.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <typeindex>

#include <entt/entt.hpp>
#include <imgui.h>
#include <yaml-cpp/yaml.h>

namespace Gravix
{

	struct ComponentSpecification
	{
		bool HasNodeTree = false;
		bool CanRemoveComponent = true;
	};

	struct ComponentUserSettings
	{
		bool RemoveComponent = false;
	};

	struct ComponentInfo
	{
		std::string Name;
		std::function<void(void*, Scene*)> OnCreateFunc;
		std::function<void(YAML::Emitter&, void*)> SerializeFunc;
		std::function<void(void*, const YAML::Node&)> DeserializeFunc;
		std::function<void(void*, ComponentUserSettings*)> ImGuiRenderFunc;

		std::function<void* (entt::registry&, entt::entity)> GetComponentFunc;
		std::function<bool(entt::registry&, entt::entity)> HasComponentFunc;

		std::function<void(entt::registry&, entt::entity)> AddComponentFunc;
		std::function<void(entt::registry&, entt::entity)> RemoveComponentFunc;
	};

	class ComponentRegistry
	{
	public:
		template<typename T>
		void RegisterComponent(const std::string& name,
			ComponentSpecification specification,
			std::function<void(T&, Scene* scene)> onCreate,
			std::function<void(YAML::Emitter&, T&)> serialize,
			std::function<void(T&, const YAML::Node&)> deserialize,
			std::function<void(T&)> imguiRender
		)
		{
			ComponentInfo info;
			info.Name = name;
			info.OnCreateFunc = [onCreate](void* instance, Scene* scene)
				{
					if (onCreate)
						onCreate(*reinterpret_cast<T*>(instance), scene);
				};
			info.SerializeFunc = [serialize, name](YAML::Emitter& out, void* instance) -> void
				{
					if (serialize)
					{
						out << YAML::Key << name + "Component" << YAML::BeginMap;
						serialize(out, *reinterpret_cast<T*>(instance));
						out << YAML::EndMap;
					}
				};
			info.DeserializeFunc = [deserialize](void* instance, const YAML::Node& node) -> void
				{
					if(deserialize)
						deserialize(*reinterpret_cast<T*>(instance), node);
				};
			info.ImGuiRenderFunc = [name, imguiRender, specification](void* instance, ComponentUserSettings* userSettings) -> void
				{
					if (!specification.HasNodeTree)
					{
						imguiRender(*reinterpret_cast<T*>(instance));
					}
					else
					{
						bool open = ImGui::TreeNodeEx((void*)typeid(*reinterpret_cast<T*>(instance)).hash_code(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap, name.c_str());
						ImGui::SameLine();
						if (ImGui::Button("+"))
						{
							ImGui::OpenPopup("ComponentSettings");
						}

						if (ImGui::BeginPopup("ComponentSettings"))
						{
							if (specification.CanRemoveComponent)
							{
								if (ImGui::Button("Remove Component"))
								{
									userSettings->RemoveComponent = true;
								}
							}

							ImGui::EndPopup();
						}

						if (open)
						{
							imguiRender(*reinterpret_cast<T*>(instance));
							ImGui::TreePop();
						}
					}
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

			info.HasComponentFunc = [](entt::registry& registry, entt::entity entity) -> bool
				{
					return registry.all_of<T>(entity);
				};

			info.AddComponentFunc = [](entt::registry& registry, entt::entity entity) -> void
				{
					registry.emplace<T>(entity);
				};

			info.RemoveComponentFunc = [](entt::registry& registry, entt::entity entity) -> void
				{
					registry.remove<T>(entity);
				};

			m_Components[typeid(T)] = info;
			m_ComponentOrder.push_back(typeid(T));
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
		const std::vector<std::type_index>& GetComponentOrder() const { return m_ComponentOrder; }
	private:
		std::unordered_map<std::type_index, ComponentInfo> m_Components;
		std::vector<std::type_index> m_ComponentOrder;
	};

}