#pragma once

#include "Scene.h"
#include "Components.h"

#include <entt/entt.hpp>

namespace Gravix 
{

	class Entity 
	{
	public:
		Entity(entt::entity handle, Scene* scene) 
			: m_EntityHandle(handle), m_Scene(scene) { }
		Entity(const Entity&) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args) 
		{
			GX_CORE_ASSERT(!HasComponent<T>(), "Entity already has componenet!");

			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent() 
		{
			GX_CORE_ASSERT(HasComponent<T>(), "Entity does not has componenet!");

			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent<T>() 
		{
			GX_CORE_ASSERT(HasComponent<T>(), "Entity does not has componenet!");

			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent() 
		{
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		glm::mat4& GetTransform() { return GetComponent<TransformComponent>(); }
		UUID& GetID() { return GetComponent<TagComponent>(); }

		const glm::mat4& GetTransform() const { return GetComponent<TransformComponent>(); }
		const UUID& GetID() const { return GetComponent<TagComponent>(); }
	private:
		entt::entity m_EntityHandle;
		Scene* m_Scene;
	};

}