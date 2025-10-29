#pragma once

#include "Renderer/Generic/Command.h"
#include "Renderer/Generic/Camera.h"

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

		void OnEditorUpdate(float ts);
		void OnRuntimeUpdate(float ts);

		void OnEditorRender(Command& cmd, const Camera& camera, const glm::mat4& viewMatrix);
		void OnRuntimeRender(Command& cmd);

		void OnViewportResize(uint32_t width, uint32_t height);
	private:
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		friend class Entity;
		friend class SceneHierarchyPanel;
	};

}