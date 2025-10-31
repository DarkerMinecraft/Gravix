#pragma once

#include "Renderer/Generic/Command.h"
#include "Renderer/Generic/Camera.h"

#include "EditorCamera.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Gravix 
{

	class Entity;

	class Scene 
	{
	public:
		Scene();
		~Scene();
		
		Entity CreateEntity(const std::string& name = std::string("Unnamed Entity"));
		void DestroyEntity(Entity entity);

		void OnEditorUpdate(float ts);
		void OnRuntimeUpdate(float ts);

		void OnEditorRender(Command& cmd, EditorCamera& camera);
		void OnRuntimeRender(Command& cmd);

		void OnViewportResize(uint32_t width, uint32_t height);

		uint32_t GetViewportWidth() const { return m_ViewportWidth; }
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }
	private:
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class InspectorPanel;
	};

}