#pragma once

#include "Scene.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <typeindex>

#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui_internal.h>
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
		bool WasModified = false;
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
					// Track if any items were edited by checking ImGui's internal state before/after rendering
					ImGuiContext& g = *ImGui::GetCurrentContext();
					ImGuiID activeIdBefore = g.ActiveId;
					bool wasEditingBefore = g.ActiveIdHasBeenEditedThisFrame;

					if (!specification.HasNodeTree)
					{
						imguiRender(*reinterpret_cast<T*>(instance));
					}
					else
					{
						// Unity-style component header
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
						ImGui::Separator();

						// Component header with Unity-style colors
						ImVec4 headerColor = ImVec4(0.196f, 0.196f, 0.196f, 1.0f);
						ImGui::PushStyleColor(ImGuiCol_Header, headerColor);
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.267f, 0.267f, 0.267f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.294f, 0.294f, 0.294f, 1.0f));

						// Use bold font for component headers
						ImGuiIO& io = ImGui::GetIO();
						ImGui::PushFont(io.Fonts->Fonts[2]); // 18px bold font for headers

						bool open = ImGui::TreeNodeEx((void*)typeid(*reinterpret_cast<T*>(instance)).hash_code(),
							ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed,
							name.c_str());

						ImGui::PopFont();
						ImGui::PopStyleColor(3);

						// Unity-style settings button (three dots)
						ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.0f);
						ImGui::AlignTextToFramePadding();
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.349f, 0.349f, 0.349f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.443f, 0.443f, 0.443f, 1.0f));

						if (ImGui::SmallButton("..."))
						{
							ImGui::OpenPopup("ComponentSettings");
						}

						ImGui::PopStyleColor(3);
						ImGui::PopStyleVar();

						// Component settings popup
						if (ImGui::BeginPopup("ComponentSettings"))
						{
							if (ImGui::MenuItem("Reset"))
							{
								// TODO: Implement component reset
							}

							if (ImGui::MenuItem("Copy Component"))
							{
								// TODO: Implement component copy
							}

							if (specification.CanRemoveComponent)
							{
								ImGui::Separator();
								if (ImGui::MenuItem("Remove Component"))
								{
									userSettings->RemoveComponent = true;
								}
							}

							ImGui::EndPopup();
						}

						// Component content
						if (open)
						{
							ImGui::Spacing();
							imguiRender(*reinterpret_cast<T*>(instance));
							ImGui::Spacing();
							ImGui::TreePop();
						}
					}

					// Detect if any item was edited during the component rendering
					// Check if an item was deactivated after being edited
					if (!wasEditingBefore && g.ActiveIdHasBeenEditedThisFrame)
					{
						userSettings->WasModified = true;
					}
					// Also check if we went from active to inactive (item deactivated)
					else if (activeIdBefore != 0 && g.ActiveId == 0)
					{
						userSettings->WasModified = true;
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