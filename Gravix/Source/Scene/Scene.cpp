#include "pch.h"
#include "Scene.h"

#include "Entity.h"

#include "Asset/AssetManager.h"
#include "Renderer/Generic/Renderer2D.h"
#include "Physics/PhysicsWorld.h"

#include "Scripting/Core/ScriptEngine.h"

namespace Gravix
{
	// Constructor and destructor defined here where PhysicsWorld is complete
	// This allows Ref<PhysicsWorld> to work with incomplete type in header
	Scene::Scene() = default;
	Scene::~Scene() = default;

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

					// Use the component registry's copy function
					if (oldComponent && newComponent && info->CopyFunc)
					{
						info->CopyFunc(newComponent, oldComponent);
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

	// Copy multi-instance components (like ScriptComponent)
	for (const auto& [entityID, componentsMap] : other->m_MultiComponents)
	{
		for (const auto& [typeIndex, instances] : componentsMap)
		{
			const ComponentInfo* info = ComponentRegistry::Get().GetComponentInfo(typeIndex);
			if (!info)
				continue;

			// Deep copy each instance
			for (const auto& instancePtr : instances)
			{
				// Create a new shared_ptr with a copy of the component
				if (typeIndex == typeid(ScriptComponent))
				{
					auto* oldScript = static_cast<ScriptComponent*>(instancePtr.get());
					auto newScript = std::make_shared<ScriptComponent>();
					newScript->Name = oldScript->Name;
					newScene->m_MultiComponents[entityID][typeIndex].push_back(newScript);
				}
				// Add other multi-instance component types here as needed
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

		m_EntityMap[uuid] = entity;

		// Manually initialize the component order with the default components
		// (AddComponent tracking happens automatically for these, but we need them in the right order)
		orderComponent.ComponentOrder = {
			typeid(TagComponent),
			typeid(TransformComponent)
		};

		return entity;
	}


	Entity Scene::FindEntityByName(std::string_view name)
	{
		for(auto entity : m_Registry.view<TagComponent>())
		{
			auto& tag = m_Registry.get<TagComponent>(entity);

			if (tag.Name == name)
			{
				return Entity{ entity, this };
			}
		}

		return Entity{ entt::null, this };
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		auto it = m_EntityMap.find(uuid);
		if (it != m_EntityMap.end())
			return Entity{ it->second, this };
		return Entity{ entt::null, this };
	}

	void Scene::DestroyEntity(Entity entity)
	{
		if (entity)
		{
			m_Registry.destroy(entity);
			m_EntityMap.erase(entity.GetID());
		}
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
		OnPhysics2DStart();
		{
			ScriptEngine::OnRuntimeStart(this);

			GX_CORE_INFO("Scene::OnRuntimeStart - Checking for entities with scripts...");
			GX_CORE_INFO("  Total entities in m_MultiComponents: {0}", m_MultiComponents.size());

			// Iterate through all entities with script components in multi-component storage
			for (const auto& [entityID, componentsMap] : m_MultiComponents)
			{
				auto it = componentsMap.find(typeid(ScriptComponent));
				if (it != componentsMap.end() && !it->second.empty())
				{
					GX_CORE_INFO("  Found entity with {0} script component(s)", it->second.size());

					// Find the entity by UUID
					auto view = m_Registry.view<TagComponent>();
					for (auto entity : view)
					{
						Entity e = { entity, this };
						if (e.GetID() == entityID)
						{
							GX_CORE_INFO("  Calling OnCreateEntity for entity: {0}", e.GetName());
							ScriptEngine::OnCreateEntity(e);
							break;
						}
					}
				}
			}

			GX_CORE_INFO("Scene::OnRuntimeStart - Finished initializing scripts");
		}
	}

	void Scene::OnRuntimeStop()
	{
		OnPhysics2DStop();
	}

	void Scene::OnEditorUpdate(float ts)
	{

	}

	void Scene::OnRuntimeUpdate(float ts)
	{
		{
			// Iterate through all entities with script components in multi-component storage
			for (const auto& [entityID, componentsMap] : m_MultiComponents)
			{
				auto it = componentsMap.find(typeid(ScriptComponent));
				if (it != componentsMap.end() && !it->second.empty())
				{
					// Find the entity by UUID
					auto view = m_Registry.view<TagComponent>();
					for (auto entity : view)
					{
						Entity e = { entity, this };
						if (e.GetID() == entityID)
						{
							ScriptEngine::OnUpdateEntity(e, ts);
							break;
						}
					}
				}
			}
		}
		OnPhysics2DUpdate();
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
					Renderer2D::DrawQuad(transform, (uint32_t)entity, sprite.Color, texture, sprite.TilingFactor);
				});
		}

		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();

			view.each([&](auto entity, auto& transform, auto& circle)
				{
					Renderer2D::DrawCircle(transform, (uint32_t)entity, circle.Color, circle.Thickness, circle.Fade);
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
					Renderer2D::DrawQuad(transform, (uint32_t)entity, sprite.Color, texture, sprite.TilingFactor);
				});
		}

		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();

			view.each([&](auto entity, auto& transform, auto& circle)
				{
					Renderer2D::DrawCircle(transform, (uint32_t)entity, circle.Color, circle.Thickness, circle.Fade);
				});
		}
		Renderer2D::EndScene(cmd);
	}

	void Scene::DuplicateEntity(Entity entity)
	{
		// Create new entity with a different name and new UUID
		std::string newName = entity.GetName() + " (Copy)";
		Entity newEntity = CreateEntity(newName);

		// Get the component order from the source entity
		if (entity.HasComponent<ComponentOrderComponent>())
		{
			auto& srcOrder = entity.GetComponent<ComponentOrderComponent>();
			auto& dstOrder = newEntity.GetComponent<ComponentOrderComponent>();

			// Copy components in the exact order they were added
			for (const auto& componentType : srcOrder.ComponentOrder)
			{
				// Skip TagComponent (already created with new UUID) and ComponentOrderComponent
				if (componentType == typeid(TagComponent) || componentType == typeid(ComponentOrderComponent))
					continue;

				const ComponentInfo* info = ComponentRegistry::Get().GetComponentInfo(componentType);
				if (!info)
					continue;

				// Check if source entity has this component
				if (!info->HasComponentFunc(m_Registry, entity))
					continue;

				// Get source component
				void* srcComponent = info->GetComponentFunc(m_Registry, entity);
				if (!srcComponent)
					continue;

				// Handle TransformComponent specially (it already exists)
				if (componentType == typeid(TransformComponent))
				{
					void* dstComponent = info->GetComponentFunc(m_Registry, newEntity);
					if (dstComponent && info->CopyFunc)
					{
						info->CopyFunc(dstComponent, srcComponent);
					}
				}
				else
				{
					// Add the component to the new entity
					info->AddComponentFunc(m_Registry, newEntity);

					// Get destination component and copy data
					void* dstComponent = info->GetComponentFunc(m_Registry, newEntity);
					if (dstComponent && info->CopyFunc)
					{
						info->CopyFunc(dstComponent, srcComponent);
					}

					// Update the component order tracking
					dstOrder.ComponentOrder.push_back(componentType);
				}
			}
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


	Ref<PhysicsWorld> Scene::GetPhysicsWorld2D()
	{
		return m_PhysicsWorld;
	}

	SceneCamera Scene::GetPrimaryCameraEntity(glm::mat4* transform)
	{
		for (auto entity : m_Registry.view<CameraComponent>())
		{
			auto& camera = m_Registry.get<CameraComponent>(entity);
			if (camera.Primary)
			{
				if (transform)
				{
					auto& tc = m_Registry.get<TransformComponent>(entity);
					*transform = tc.Transform;
				}
				return camera.Camera;
			}
		}
		return SceneCamera();
	}


	void Scene::OnPhysics2DStart()
	{
		m_PhysicsWorld = CreateRef<PhysicsWorld>();

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

			if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto& circleCollider = entity.GetComponent<CircleCollider2DComponent>();
				uint64_t shape = m_PhysicsWorld->CreateCircleShape(bodyID, transform, circleCollider);
				circleCollider.RuntimeShape = shape;
			}
		}
	}


	void Scene::OnPhysics2DUpdate()
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

	void Scene::OnPhysics2DStop()
	{
		m_PhysicsWorld = nullptr; // Automatically cleaned up by Ref<>
	}

}
