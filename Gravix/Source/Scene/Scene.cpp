#include "pch.h"
#include "Scene.h"

#include "Entity.h"

#include "Renderer/Generic/Renderer2D.h"

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

	void Scene::DestroyEntity(Entity entity)
	{
		if (entity) 
		{
			m_Registry.destroy(entity);
		}
	}

	void Scene::OnEditorUpdate(float ts)
	{

	}

	void Scene::OnRuntimeUpdate(float ts)
	{

	}

	void Scene::OnEditorRender(Command& cmd, EditorCamera& camera)
	{
		{
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);

			Renderer2D::BeginScene(cmd, camera.GetViewProjection());
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawQuad(transform, sprite);
			}
			Renderer2D::EndScene(cmd);
		}
	}

	void Scene::OnRuntimeRender(Command& cmd)
	{
		Camera* mainCamera = nullptr;
		glm::mat4* cameraTransform = nullptr;
		{
			auto group = m_Registry.group<TransformComponent, CameraComponent>();
			for (auto entity : group) 
			{
				auto [transform, camera] = group.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary) 
				{
					mainCamera = &camera.Camera;
					cameraTransform = &transform.Transform;
				}
			}
		}

		{
			if (mainCamera) 
			{
				auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);

				Renderer2D::BeginScene(cmd, mainCamera->GetProjection(), *cameraTransform);
				for (auto entity : group)
				{
					auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

					Renderer2D::DrawQuad(transform, sprite);
				}
				Renderer2D::EndScene(cmd);
			}
		}

		delete mainCamera;
		delete cameraTransform;
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		if (m_ViewportWidth != width || m_ViewportHeight != height)
		{
			m_ViewportWidth = width;
			m_ViewportHeight = height;

			auto view = m_Registry.view<CameraComponent>();
			for (auto entity : view) 
			{
				auto& cameraComponent = view.get<CameraComponent>(entity);
				if (!cameraComponent.FixedAspectRatio) 
				{
					cameraComponent.Camera.SetViewportSize(width, height);
				}
			}
		}
	}

}