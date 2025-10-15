#include "pch.h"
#include "Scene.h"

#include "Entity.h"

namespace Gravix 
{

	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };

		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();

		return entity;
	}

	void Scene::OnEditorUpdate(float ts)
	{

	}

	void Scene::OnRuntimeUpdate(float ts)
	{

	}

	void Scene::OnEditorRender(Command& cmd)
	{

	}

	void Scene::OnRuntimeRender(Command& cmd)
	{

	}
}