#include "pch.h"
#include "Scene.h"

#include "Entity.h"

#include "Asset/AssetManager.h"

#include "Renderer/Generic/Renderer2D.h"

namespace Gravix
{

	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

	}

	Entity Scene::CreateEntity(const std::string& name, UUID uuid, uint32_t creationIndex)
	{
		Entity entity = { m_Registry.create(), this };

		// Add ComponentOrderComponent first
		auto& orderComponent = m_Registry.emplace<ComponentOrderComponent>(entity);

		// Add default components
		entity.AddComponent<TagComponent>(name, uuid, creationIndex == (uint32_t)-1 ? m_NextCreationIndex++ : creationIndex);
		entity.AddComponent<TransformComponent>();

		// Manually initialize the component order with the default components
		// (AddComponent tracking happens automatically for these, but we need them in the right order)
		orderComponent.ComponentOrder = {
			typeid(TagComponent),
			typeid(TransformComponent)
		};

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		if (entity)
			m_Registry.destroy(entity);
	}

	void Scene::ExtractSceneDependencies(std::vector<AssetHandle>* outDependencies) const
	{
		std::vector<AssetHandle>& outDependenciesRef = outDependencies ? *outDependencies : *(new std::vector<AssetHandle>());
		auto view = m_Registry.view<SpriteRendererComponent>();
		for (auto entity : view)
		{
			auto& sprite = view.get<SpriteRendererComponent>(entity);
			if (sprite.Texture != 0)
			{
				outDependenciesRef.push_back(sprite.Texture);
			}
		}

		std::sort(outDependenciesRef.begin(), outDependenciesRef.end());
		outDependenciesRef.erase(std::unique(outDependenciesRef.begin(), outDependenciesRef.end()), outDependenciesRef.end());

		outDependencies = &outDependenciesRef;
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
			Renderer2D::BeginScene(cmd, camera);

			auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
			view.each([&](auto entity, auto& transform, auto& sprite) {
				Ref<Texture2D> texture = sprite.Texture == 0 ? nullptr : AssetManager::GetAsset<Texture2D>(sprite.Texture);
				Renderer2D::DrawQuad(transform, (uint64_t)(uint32_t)entity, sprite, texture, sprite);
			});

			Renderer2D::EndScene(cmd);
		}
	}

	void Scene::OnRuntimeRender(Command& cmd)
	{
		Camera mainCamera;
		glm::mat4 cameraTransform;
		bool foundCamera = false;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			view.each([&](auto entity, auto& transform, auto& camera) {
				if (camera.Primary)
				{
					mainCamera = camera.Camera;

					// Calculate camera transform without scale (cameras should only use position and rotation)
					glm::vec3 radianRotation = { glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) };
					glm::mat4 rotation = glm::toMat4(glm::quat(radianRotation));
					cameraTransform = glm::translate(glm::mat4(1.0f), transform.Position) * rotation;

					foundCamera = true;
				}
			});
		}

		if (foundCamera)
		{
			Renderer2D::BeginScene(cmd, mainCamera, cameraTransform);

			auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
			view.each([&](auto entity, auto& transform, auto& sprite) {
				Ref<Texture2D> texture = sprite.Texture == 0 ? nullptr : AssetManager::GetAsset<Texture2D>(sprite.Texture);
				Renderer2D::DrawQuad(transform, -1, sprite, texture, sprite);
			});

			Renderer2D::EndScene(cmd);
		}
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