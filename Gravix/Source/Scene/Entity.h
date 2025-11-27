#pragma once

#include "Scene.h"

#include "Components.h"
#include "ComponentRegistry.h"

#include <typeindex>
#include <algorithm>
#include <entt/entt.hpp>

namespace Gravix
{

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene)
			: m_EntityHandle(handle), m_Scene(scene) {
		}
		Entity(const Entity&) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			GX_ASSERT(!HasComponent<T>(), "Entity already has componenet!");

			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);

			if (auto* info = ComponentRegistry::Get().GetComponentInfo(typeid(T)))
			{
				if (info->OnCreateFunc)
					info->OnCreateFunc(&component, m_Scene);
			}

			// Track component order (skip for ComponentOrderComponent itself to avoid recursion)
			if constexpr (!std::is_same_v<T, ComponentOrderComponent>)
			{
				if (HasComponent<ComponentOrderComponent>())
				{
					auto& order = GetComponent<ComponentOrderComponent>();
					order.ComponentOrder.push_back(typeid(T));
				}
			}

			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
			if (auto* info = ComponentRegistry::Get().GetComponentInfo(typeid(T)))
			{
				if (info->OnCreateFunc)
					info->OnCreateFunc(&component, m_Scene);
			}
			// Track component order (skip for ComponentOrderComponent itself to avoid recursion)
			if constexpr (!std::is_same_v<T, ComponentOrderComponent>)
			{
				if (HasComponent<ComponentOrderComponent>())
				{
					auto& order = GetComponent<ComponentOrderComponent>();
					// If the component is not already in the order list, add it
					if (std::find(order.ComponentOrder.begin(), order.ComponentOrder.end(), typeid(T)) == order.ComponentOrder.end())
					{
						order.ComponentOrder.push_back(typeid(T));
					}
				}
			}
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			GX_ASSERT(HasComponent<T>(), "Entity does not has componenet!");

			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		const T& GetComponent() const
		{
			GX_ASSERT(HasComponent<T>(), "Entity does not has componenet!");

			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			GX_ASSERT(HasComponent<T>(), "Entity does not has componenet!");

			m_Scene->m_Registry.remove<T>(m_EntityHandle);

			// Remove from component order tracking
			if constexpr (!std::is_same_v<T, ComponentOrderComponent>)
			{
				if (HasComponent<ComponentOrderComponent>())
				{
					auto& order = GetComponent<ComponentOrderComponent>();
					auto& vec = order.ComponentOrder;
					vec.erase(std::remove(vec.begin(), vec.end(), typeid(T)), vec.end());
				}
			}
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		template<typename T>
		const bool HasComponent() const
		{
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		bool HasComponent(std::type_index typeIndex)
		{
			const ComponentInfo* info = ComponentRegistry::Get().GetComponentInfo(typeIndex);
			bool hasInRegistry = info->GetComponentFunc(m_Scene->m_Registry, m_EntityHandle) != nullptr;

			// Also check multi-component storage
			if (!hasInRegistry && info->Specification.AllowMultiple)
			{
				UUID entityID = GetID();
				auto entityIt = m_Scene->m_MultiComponents.find(entityID);
				if (entityIt != m_Scene->m_MultiComponents.end())
				{
					auto compIt = entityIt->second.find(typeIndex);
					if (compIt != entityIt->second.end() && !compIt->second.empty())
					{
						return true;
					}
				}
			}

			return hasInRegistry;
		}

		void* GetComponent(std::type_index typeIndex)
		{
			GX_ASSERT(HasComponent(typeIndex), "Entity does not has componenet!");

			const ComponentInfo* info = ComponentRegistry::Get().GetComponentInfo(typeIndex);
			return info->GetComponentFunc(m_Scene->m_Registry, m_EntityHandle);
		}

		void AddComponent(std::type_index typeIndex)
		{
			const ComponentInfo* info = ComponentRegistry::Get().GetComponentInfo(typeIndex);
			GX_ASSERT(info, "Component type not registered!");

			// Multi-instance components should use AddComponentInstance<T>() instead
			GX_ASSERT(!info->Specification.AllowMultiple, "Cannot add multi-instance component through AddComponent(type_index). Use AddComponentInstance<T>() instead!");

			GX_ASSERT(info->AddComponentFunc, "Component has no AddComponentFunc!");
			GX_ASSERT(!HasComponent(typeIndex), "Entity already has componenet!");

			info->AddComponentFunc(m_Scene->m_Registry, m_EntityHandle);
			if (info->OnCreateFunc)
			{
				void* component = info->GetComponentFunc(m_Scene->m_Registry, m_EntityHandle);
				info->OnCreateFunc(component, m_Scene);
			}

			// Track component order (skip for ComponentOrderComponent itself)
			if (typeIndex != typeid(ComponentOrderComponent))
			{
				if (HasComponent<ComponentOrderComponent>())
				{
					auto& order = GetComponent<ComponentOrderComponent>();
					order.ComponentOrder.push_back(typeIndex);
				}
			}
		}

		void RemoveComponent(std::type_index typeIndex)
		{
			const ComponentInfo* info = ComponentRegistry::Get().GetComponentInfo(typeIndex);
			GX_ASSERT(info, "Component type not registered!");
			GX_ASSERT(info->RemoveComponentFunc, "Component has no RemoveComponentFunc!");
			GX_ASSERT(HasComponent(typeIndex), "Entity does not have this component!");

			info->RemoveComponentFunc(m_Scene->m_Registry, m_EntityHandle);

			// Remove from component order tracking
			if (typeIndex != typeid(ComponentOrderComponent))
			{
				if (HasComponent<ComponentOrderComponent>())
				{
					auto& order = GetComponent<ComponentOrderComponent>();
					auto& vec = order.ComponentOrder;
					vec.erase(std::remove(vec.begin(), vec.end(), typeIndex), vec.end());
				}
			}
		}

		// Multi-instance component support
		template<typename T>
		std::vector<T*> GetComponents()
		{
			std::vector<T*> result;
			UUID entityID = GetID();

			auto entityIt = m_Scene->m_MultiComponents.find(entityID);
			if (entityIt != m_Scene->m_MultiComponents.end())
			{
				auto compIt = entityIt->second.find(typeid(T));
				if (compIt != entityIt->second.end())
				{
					for (auto& compPtr : compIt->second)
					{
						result.push_back(static_cast<T*>(compPtr.get()));
					}
				}
			}

			return result;
		}

		template<typename T, typename... Args>
		T& AddComponentInstance(Args&&... args)
		{
			auto* info = ComponentRegistry::Get().GetComponentInfo(typeid(T));
			GX_ASSERT(info && info->Specification.AllowMultiple, "Component must have AllowMultiple=true!");

			UUID entityID = GetID();
			auto componentPtr = std::make_shared<T>(std::forward<Args>(args)...);
			T* component = componentPtr.get();

			m_Scene->m_MultiComponents[entityID][typeid(T)].push_back(componentPtr);

			if (info->OnCreateFunc)
				info->OnCreateFunc(component, m_Scene);

			return *component;
		}

		template<typename T>
		void RemoveComponentInstance(int index)
		{
			UUID entityID = GetID();
			auto entityIt = m_Scene->m_MultiComponents.find(entityID);
			if (entityIt != m_Scene->m_MultiComponents.end())
			{
				auto compIt = entityIt->second.find(typeid(T));
				if (compIt != entityIt->second.end() && index >= 0 && index < compIt->second.size())
				{
					compIt->second.erase(compIt->second.begin() + index);
				}
			}
		}

		template<typename T>
		int GetComponentCount()
		{
			UUID entityID = GetID();
			auto entityIt = m_Scene->m_MultiComponents.find(entityID);
			if (entityIt != m_Scene->m_MultiComponents.end())
			{
				auto compIt = entityIt->second.find(typeid(T));
				if (compIt != entityIt->second.end())
				{
					return compIt->second.size();
				}
			}
			return 0;
		}

		glm::mat4& GetTransform() { return GetComponent<TransformComponent>(); }
		UUID& GetID() { return GetComponent<TagComponent>(); }
		std::string& GetName() { return GetComponent<TagComponent>(); }

		const glm::mat4& GetTransform() const { return GetComponent<TransformComponent>(); }
		const UUID& GetID() const { return GetComponent<TagComponent>(); }
		const std::string& GetName() const { return GetComponent<TagComponent>(); }

		Scene* GetScene() const { return m_Scene; }

		operator bool() const { return m_EntityHandle != entt::null; }
		operator uint64_t() const { return (uint64_t)GetID(); }
		operator entt::entity() const { return m_EntityHandle; }

		bool operator==(const Entity& other) const
		{
			return GetID() == other.GetID() && m_Scene == other.m_Scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}
	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene;
	};

}