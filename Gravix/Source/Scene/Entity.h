#pragma once

#include "Scene.h"

#include "Components.h"
#include "ComponentRegistry.h"

#include <entt/entt.hpp>

namespace Gravix
{

	class Entity
	{
	public:
		Entity(entt::entity handle, Scene* scene)
			: m_EntityHandle(handle), m_Scene(scene) {}
		Entity(const Entity&) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			GX_CORE_ASSERT(!HasComponent<T>(), "Entity already has componenet!");

			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);

			if (auto* info = ComponentRegistry::Get().GetComponentInfo(typeid(T)))
			{
				if (info->OnCreateFunc)
					info->OnCreateFunc(&component, m_Scene);
			}

			m_AddedComponents.push_back(std::type_index(typeid(T)));

			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			GX_CORE_ASSERT(HasComponent<T>(), "Entity does not has componenet!");

			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		const T& GetComponent() const
		{
			GX_CORE_ASSERT(HasComponent<T>(), "Entity does not has componenet!");

			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			GX_CORE_ASSERT(HasComponent<T>(), "Entity does not has componenet!");

			m_Scene->m_Registry.remove<T>(m_EntityHandle);

			auto it = std::find(m_AddedComponents.begin(), m_AddedComponents.end(), std::type_index(typeid(T)));
			if (it != m_AddedComponents.end())
				m_AddedComponents.erase(it);
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

		const std::vector<std::type_index> GetAddedComponents() const { return m_AddedComponents; }

		glm::mat4& GetTransform() { return GetComponent<TransformComponent>(); }
		UUID& GetID() { return GetComponent<TagComponent>(); }
		std::string& GetName() { return GetComponent<TagComponent>(); }

		const glm::mat4& GetTransform() const { return GetComponent<TransformComponent>(); }
		const UUID& GetID() const { return GetComponent<TagComponent>(); }
		const std::string& GetName() const { return GetComponent<TagComponent>(); }

		operator bool() const { return m_EntityHandle != entt::null; }
		operator uint64_t() const { return (uint64_t) GetID(); }

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

		std::vector<std::type_index> m_AddedComponents;
	};

}