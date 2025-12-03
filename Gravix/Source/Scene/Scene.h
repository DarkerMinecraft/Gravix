#pragma once

#include "Renderer/Generic/Command.h"
#include "Renderer/Generic/Camera.h"
#include "EditorCamera.h"

#include "Core/UUID.h"

#include "Asset/Asset.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <typeindex>
#include "SceneCamera.h"

namespace Gravix
{

	class Entity;
	class PhysicsWorld;

	/**
	 * @brief Container for game objects and their components
	 *
	 * A Scene represents a collection of entities with their components, forming
	 * a complete game world or level. The scene manages:
	 * - Entity lifecycle (creation, destruction, duplication)
	 * - Component storage via EnTT registry (ECS pattern)
	 * - Physics world integration (Box2D)
	 * - Update and render loops (runtime vs editor)
	 * - Viewport sizing for cameras
	 * - Asset dependencies (textures, materials used in scene)
	 *
	 * Scenes are assets that can be:
	 * - Saved/loaded via SceneSerializer
	 * - Duplicated via Copy()
	 * - Included in asset dependency graphs
	 *
	 * The scene has two operational modes:
	 * 1. **Editor Mode** - Editable scene with EditorCamera
	 * 2. **Runtime Mode** - Active game simulation with physics and scripts
	 *
	 * @par Usage Example:
	 * @code
	 * // Create a new scene
	 * Ref<Scene> scene = CreateRef<Scene>();
	 *
	 * // Create entities and add components
	 * Entity player = scene->CreateEntity("Player");
	 * player.AddComponent<TransformComponent>();
	 * player.AddComponent<SpriteRendererComponent>();
	 *
	 * // Start runtime simulation
	 * scene->OnRuntimeStart();
	 *
	 * // Update loop
	 * while (running)
	 * {
	 *     scene->OnRuntimeUpdate(deltaTime);
	 *     Command cmd;
	 *     scene->OnRuntimeRender(cmd);
	 * }
	 *
	 * // Stop simulation
	 * scene->OnRuntimeStop();
	 * @endcode
	 *
	 * @see Entity, Components, SceneSerializer
	 */
	class Scene : public Asset
	{
	public:
		/**
		 * @brief Default constructor
		 * Defined in .cpp to allow Ref<PhysicsWorld> to work with incomplete type
		 */
		Scene();

		/**
		 * @brief Default destructor
		 * Defined in .cpp to allow Ref<PhysicsWorld> to work with incomplete type
		 */
		~Scene();

		/**
		 * @brief Create a deep copy of a scene
		 * @param other Scene to copy
		 * @return New scene with duplicated entities and components
		 *
		 * Creates a complete copy including all entities, components,
		 * and scene settings. Entity UUIDs are preserved.
		 */
		static Ref<Scene> Copy(Ref<Scene> other);

		/**
		 * @brief Get the asset type
		 * @return AssetType::Scene
		 */
		virtual AssetType GetAssetType() const override { return AssetType::Scene; }

		/**
		 * @brief Create a new entity in the scene
		 * @param name Display name for the entity
		 * @param uuid Unique identifier (generated if not provided)
		 * @param creationIndex Order index (auto-assigned if -1)
		 * @return Entity handle for the created entity
		 *
		 * Creates a new entity with a TagComponent containing the name and UUID.
		 * Additional components can be added via Entity::AddComponent<T>().
		 */
		Entity CreateEntity(const std::string& name = std::string("Unnamed Entity"), UUID uuid = UUID(), uint32_t creationIndex = (uint32_t)-1);

		Entity FindEntityByName(std::string_view name);

		Entity GetEntityByUUID(UUID uuid);
		/**
		 * @brief Remove an entity and all its components from the scene
		 * @param entity Entity to destroy
		 *
		 * Destroys the entity and cleans up all associated components.
		 * The entity handle becomes invalid after this call.
		 */
		void DestroyEntity(Entity entity);

		/**
		 * @brief Extract all asset dependencies used by this scene
		 * @param outDependencies Output vector to store AssetHandles
		 *
		 * Collects all assets referenced by entities (textures, materials, etc.)
		 * for dependency tracking and asset packaging.
		 */
		void ExtractSceneDependencies(std::vector<AssetHandle>* outDependencies) const;

		/**
		 * @brief Initialize runtime systems (physics, scripts)
		 *
		 * Prepares the scene for gameplay:
		 * - Creates physics world and rigid bodies
		 * - Initializes scripting components
		 * - Sets runtime-specific state
		 */
		void OnRuntimeStart();

		/**
		 * @brief Shutdown runtime systems
		 *
		 * Stops gameplay simulation and cleans up runtime resources:
		 * - Destroys physics world
		 * - Unloads script instances
		 */
		void OnRuntimeStop();

		/**
		 * @brief Update scene in editor mode
		 * @param ts Delta time in seconds
		 *
		 * Updates editor-specific systems without running gameplay logic.
		 * Does not step physics or update scripts.
		 */
		void OnEditorUpdate(float ts);

		/**
		 * @brief Update scene in runtime mode
		 * @param ts Delta time in seconds
		 *
		 * Updates gameplay systems:
		 * - Steps physics simulation
		 * - Executes script updates
		 * - Updates animations, particles, etc.
		 */
		void OnRuntimeUpdate(float ts);

		/**
		 * @brief Render scene in editor mode
		 * @param cmd Rendering command buffer
		 * @param camera Editor camera for view/projection
		 *
		 * Renders visible entities from editor camera perspective.
		 */
		void OnEditorRender(Command& cmd, EditorCamera& camera);

		/**
		 * @brief Render scene in runtime mode
		 * @param cmd Rendering command buffer
		 *
		 * Renders the scene from the active game camera.
		 * Uses camera component from entities to determine view.
		 */
		void OnRuntimeRender(Command& cmd);

		/**
		 * @brief Create a duplicate of an entity within the scene
		 * @param entity Entity to duplicate
		 *
		 * Creates a new entity with copies of all components from the
		 * source entity. Generates a new UUID for the duplicate.
		 */
		void DuplicateEntity(Entity entity);

		/**
		 * @brief Notify scene of viewport size change
		 * @param width New viewport width in pixels
		 * @param height New viewport height in pixels
		 *
		 * Updates scene cameras and rendering systems when the
		 * viewport is resized (e.g., window resize).
		 */
		void OnViewportResize(uint32_t width, uint32_t height);

		/**
		 * @brief Get current viewport width
		 * @return Width in pixels
		 */
		uint32_t GetViewportWidth() const { return m_ViewportWidth; }

		/**
		 * @brief Get current viewport height
		 * @return Height in pixels
		 */
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }

		Ref<PhysicsWorld> GetPhysicsWorld2D(); 

		/**
		 * @brief Get the primary camera entity in the scene
		 * @return Entity with CameraComponent marked as primary
		 *
		 * Searches for the entity with a CameraComponent that is
		 * designated as the primary camera for rendering.
		 */
		SceneCamera GetPrimaryCameraEntity(glm::mat4* transform);

		/**
		 * @brief Get all entities with specified components
		 * @tparam Component Component types to filter by
		 * @return EnTT view of matching entities
		 *
		 * Returns a view that can be iterated over to access all entities
		 * that have the specified component types.
		 */
		template<typename... Component>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Component...>();
		}
	private:
		void OnPhysics2DStart();
		void OnPhysics2DUpdate();
		void OnPhysics2DStop();
	private:
		entt::registry m_Registry;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		uint32_t m_NextCreationIndex = 0;

		Ref<PhysicsWorld> m_PhysicsWorld;

		std::unordered_map<UUID, entt::entity> m_EntityMap;

		// Storage for components with AllowMultiple=true
		// Maps: Entity UUID -> Component Type -> Vector of component instances
		std::unordered_map<UUID, std::unordered_map<std::type_index, std::vector<std::shared_ptr<void>>>> m_MultiComponents;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
		friend class InspectorPanel;
	};

}