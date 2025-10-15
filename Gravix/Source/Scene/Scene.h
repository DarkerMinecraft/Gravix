#pragma once

#include "Renderer/Generic/Command.h"

#include <entt/entt.hpp>

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

		void OnEditorRender(Command& cmd);
		void OnRuntimeRender(Command& cmd);
	private:
		entt::registry m_Registry;

		friend class Entity;
	};

}