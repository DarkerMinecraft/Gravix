#include "pch.h"
#include "Scene.h"

#include "Entity.h"

#include "Asset/AssetManager.h"
#include "Renderer/Generic/Renderer2D.h"
#include "Physics/PhysicsWorld.h"

namespace Gravix
{

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = CreateRef<Scene>();

		// Copy viewport dimensions
		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		// Copy the next creation index to maintain entity creation order
		newScene->m_NextCreationIndex = other->m_NextCreationIndex;

		// Map to track old entity handle to new entity (for potential future use like parent-child relationships)
		std::unordered_map<UUID, entt::entity> entityMap;

		// Copy all entities
		auto view = other->m_Registry.view<TagComponent>();
		for (auto oldEntityHandle : view)
		{
			Entity oldEntity = { oldEntityHandle, other.get() };
			auto& oldTag = oldEntity.GetComponent<TagComponent>();

			// Create new entity with the same UUID, name, and creation index
			Entity newEntity = newScene->CreateEntity(oldTag.Name, oldTag.ID, oldTag.CreationIndex);
			entityMap[oldTag.ID] = newEntity;

			// Get the component order from the old entity (if it exists)
			if (oldEntity.HasComponent<ComponentOrderComponent>())
			{
				auto& oldOrder = oldEntity.GetComponent<ComponentOrderComponent>();
				auto& newOrder = newEntity.GetComponent<ComponentOrderComponent>();

				// Copy components in the exact order they were added
				for (const auto& componentType : oldOrder.ComponentOrder)
				{
					// Skip TagComponent and TransformComponent as they're already added by CreateEntity
					if (componentType == typeid(TagComponent) || componentType == typeid(TransformComponent))
						continue;

					// Skip ComponentOrderComponent itself
					if (componentType == typeid(ComponentOrderComponent))
						continue;

					// Get component info from registry
					const ComponentInfo* info = ComponentRegistry::Get().GetComponentInfo(componentType);
					if (!info)
						continue;

					// Check if old entity has this component
					if (!info->HasComponentFunc(other->m_Registry, oldEntityHandle))
						continue;

					// Add the component to the new entity
					info->AddComponentFunc(newScene->m_Registry, newEntity);

					// Get both component instances
					void* oldComponent = info->GetComponentFunc(other->m_Registry, oldEntityHandle);
					void* newComponent = info->GetComponentFunc(newScene->m_Registry, newEntity);

					// Copy component data based on type
					if (componentType == typeid(SpriteRendererComponent))
					{
						auto* oldSprite = static_cast<SpriteRendererComponent*>(oldComponent);
						auto* newSprite = static_cast<SpriteRendererComponent*>(newComponent);
						newSprite->Color = oldSprite->Color;
						newSprite->Texture = oldSprite->Texture;
						newSprite->TilingFactor = oldSprite->TilingFactor;
					}
					else if (componentType == typeid(CameraComponent))
					{
						auto* oldCamera = static_cast<CameraComponent*>(oldComponent);
						auto* newCamera = static_cast<CameraComponent*>(newComponent);
						newCamera->Camera = oldCamera->Camera;
						newCamera->Primary = oldCamera->Primary;
						newCamera->FixedAspectRatio = oldCamera->FixedAspectRatio;
					}
					else if (componentType == typeid(Rigidbody2DComponent))
					{
						auto* oldRb = static_cast<Rigidbody2DComponent*>(oldComponent);
						auto* newRb = static_cast<Rigidbody2DComponent*>(newComponent);
						newRb->Type = oldRb->Type;
						newRb->FixedRotation = oldRb->FixedRotation;
						// Don't copy RuntimeBody - it's runtime only
						newRb->RuntimeBody = 0;
					}
					else if (componentType == typeid(BoxCollider2DComponent))
					{
						auto* oldCollider = static_cast<BoxCollider2DComponent*>(oldComponent);
						auto* newCollider = static_cast<BoxCollider2DComponent*>(newComponent);
						newCollider->Offset = oldCollider->Offset;
						newCollider->Size = oldCollider->Size;
						newCollider->Density = oldCollider->Density;
						newCollider->Friction = oldCollider->Friction;
						newCollider->Restitution = oldCollider->Restitution;
						// Don't copy RuntimeShape - it's runtime only
						newCollider->RuntimeShape = 0;
					}

					// Update the component order tracking
					newOrder.ComponentOrder.push_back(componentType);
				}

				// Also copy the TransformComponent data (it was already added by CreateEntity)
				if (oldEntity.HasComponent<TransformComponent>())
				{
					auto& oldTransform = oldEntity.GetComponent<TransformComponent>();
					auto& newTransform = newEntity.GetComponent<TransformComponent>();
					newTransform.Position = oldTransform.Position;
					newTransform.Rotation = oldTransform.Rotation;
					newTransform.Scale = oldTransform.Scale;
					newTransform.CalculateTransform();
				}
			}
		}

		return newScene;
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
		if (!outDependencies)
			return;
		
		auto view = m_Registry.view<SpriteRendererComponent>();
		for (auto entity : view)
		{
			auto& sprite = view.get<SpriteRendererComponent>(entity);
			if (sprite.Texture != 0)
			{
				outDependencies->push_back(sprite.Texture);
			}
		}

		std::sort(outDependencies->begin(), outDependencies->end());
		outDependencies->erase(std::unique(outDependencies->begin(), outDependencies->end()), outDependencies->end());
	}
	void Scene::OnRuntimeStart()
	{
		m_PhysicsWorld = new PhysicsWorld();

		{
			auto view = m_Registry.view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();

				auto& rb2d = view.get<Rigidbody2DComponent>(e);
				uint64_t bodyID = m_PhysicsWorld->CreateBody(transform, rb2d);
				rb2d.RuntimeBody = bodyID;

				if (entity.HasComponent<BoxCollider2DComponent>())
				{
					auto& boxCollider = entity.GetComponent<BoxCollider2DComponent>();
					uint64_t shape = m_PhysicsWorld->CreateBoxShape(bodyID, transform, boxCollider);
					boxCollider.RuntimeShape = shape;
				}
			}
		}
	}

	void Scene::OnRuntimeStop()
	{
		delete m_PhysicsWorld;
		m_PhysicsWorld = nullptr;
	}

	void Scene::OnEditorUpdate(float ts)
	{

	}

	void Scene::OnRuntimeUpdate(float ts)
	{
		// SCRIPTS FIRST

		{
			m_PhysicsWorld->Step(1.0f / 60.0f, 4);

			auto view = m_Registry.view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
				if (rb2d.RuntimeBody)
				{
					uint64_t bodyID = rb2d.RuntimeBody;
					glm::vec2 position = m_PhysicsWorld->GetBodyPosition(bodyID);
					float rotation = m_PhysicsWorld->GetBodyRotation(bodyID);

					transform.Position.x = position.x;
					transform.Position.y = position.y;
					transform.Rotation.z = rotation;

					// Recalculate transform matrix after updating from physics
					transform.CalculateTransform();
				}
			}
		}
	}

	void Scene::OnEditorRender(Command& cmd, EditorCamera& camera)
	{
		GX_PROFILE_FUNCTION();

		Renderer2D::BeginScene(cmd, camera);

		{
			auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();

			view.each([&](auto entity, auto& transform, auto& sprite)
				{
					Ref<Texture2D> texture = sprite.Texture == 0 ? nullptr : AssetManager::GetAsset<Texture2D>(sprite.Texture);
					Renderer2D::DrawQuad(transform, (uint64_t)(uint32_t)entity, sprite, texture, sprite);
				});
		}

		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();

			view.each([&](auto entity, auto& transform, auto& circle)
				{
					Renderer2D::DrawCircle(transform, (uint64_t)(uint32_t)entity, circle.Color, circle.Thickness, circle.Fade);
				});
		}

		Renderer2D::EndScene(cmd);
	}

	void Scene::OnRuntimeRender(Command& cmd)
	{
		GX_PROFILE_FUNCTION();

		Camera mainCamera;
		glm::mat4 cameraTransform;
		bool foundCamera = false;

		// Find primary camera
		auto cameraView = m_Registry.view<TransformComponent, CameraComponent>();
		cameraView.each([&](auto entity, auto& transform, auto& camera)
			{
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

		if (!foundCamera)
			return;  // Early exit if no camera

		Renderer2D::BeginScene(cmd, mainCamera, cameraTransform);
		{
			auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();

			view.each([&](auto entity, auto& transform, auto& sprite)
				{
					Ref<Texture2D> texture = sprite.Texture == 0 ? nullptr : AssetManager::GetAsset<Texture2D>(sprite.Texture);
					Renderer2D::DrawQuad(transform, (uint64_t)(uint32_t)entity, sprite, texture, sprite);
				});
		}

		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();

			view.each([&](auto entity, auto& transform, auto& circle)
				{
					Renderer2D::DrawCircle(transform, (uint64_t)(uint32_t)entity, circle.Color, circle.Thickness, circle.Fade);
				});
		}
		Renderer2D::EndScene(cmd);
	}

	template<typename Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		if (src.HasComponent<Component>())
		{
			const auto& srcComponent = src.GetComponent<Component>();

			if (dst.HasComponent<Component>())
			{
				// Component already exists (like TagComponent or TransformComponent), so just copy the data
				dst.GetComponent<Component>() = srcComponent;
			}
			else
			{
				// Component doesn't exist, add it with the copied data
				dst.AddComponent<Component>(srcComponent);
			}
		}
	}

	void Scene::DuplicateEntity(Entity entity)
	{
		// Create new entity with a different name and new UUID
		std::string newName = entity.GetName() + " (Copy)";
		Entity newEntity = CreateEntity(newName);

		// Copy all components except TagComponent (already set with different name/UUID)
		CopyComponentIfExists<TransformComponent>(newEntity, entity);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
		CopyComponentIfExists<CircleRendererComponent>(newEntity, entity);
		CopyComponentIfExists<CameraComponent>(newEntity, entity);
		CopyComponentIfExists<Rigidbody2DComponent>(newEntity, entity);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity, entity);
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