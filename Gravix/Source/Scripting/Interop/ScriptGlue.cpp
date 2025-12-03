#include "pch.h"
#include "ScriptGlue.h"
#include "Scripting/Core/ScriptEngine.h"

#include "Physics/PhysicsWorld.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Scene/ComponentRegistry.h"

#include "Core/UUID.h"
#include "Core/Input.h"
#include "Core/Console.h"

#include <mono/metadata/loader.h>
#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>

#include <typeindex>

#define GX_ADD_INTERNAL_CALL(name) mono_add_internal_call("GravixEngine.InternalCalls::"#name, name) 
namespace Gravix
{

	std::unordered_map<MonoType*, std::type_index> s_MonoTypeToTypeIndex;

	#pragma region Input

	static bool Input_IsKeyDown(Key key)
	{
		return Input::IsKeyDown(key);
	}

	static bool Input_IsKeyPressed(Key key)
	{
		return Input::IsKeyPressed(key);
	}

	#pragma endregion

	#pragma region Entity
	static bool Entity_HasComponent(UUID entityID, MonoReflectionType* componentType)
	{
		MonoType* monoComponentType = mono_reflection_type_get_type(componentType);
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		auto it = s_MonoTypeToTypeIndex.find(monoComponentType);
		GX_ASSERT(it != s_MonoTypeToTypeIndex.end(), "Component not registered with ScriptGlue!");

		return entity.HasComponent(it->second);
	}

	static void Entity_AddComponent(UUID entityID, MonoReflectionType* componentType)
	{
		MonoType* monoComponentType = mono_reflection_type_get_type(componentType);
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		auto it = s_MonoTypeToTypeIndex.find(monoComponentType);
		GX_ASSERT(it != s_MonoTypeToTypeIndex.end(), "Component not registered with ScriptGlue!");

		entity.AddComponent(it->second);
	}

	static void Entity_RemoveComponent(UUID entityID, MonoReflectionType* componentType)
	{
		MonoType* monoComponentType = mono_reflection_type_get_type(componentType);
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		auto it = s_MonoTypeToTypeIndex.find(monoComponentType);
		GX_ASSERT(it != s_MonoTypeToTypeIndex.end(), "Component not registered with ScriptGlue!");

		entity.RemoveComponent(it->second);
	}

	static uint64_t Entity_FindEntityByName(MonoString* name)
	{
		if (!name)
			return 0;

		char* cName = mono_string_to_utf8(name);
		if (!cName)
			return 0;

		std::string strName(cName);
		mono_free(cName);

		if (strName.empty())
			return 0;

		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->FindEntityByName(strName);

		if (!entity)
			return 0;

		return (uint64_t)entity.GetID();
	}

	static MonoObject* Entity_GetScriptInstance(UUID entityID, MonoReflectionType* scriptType)
	{
		if (!scriptType)
			return nullptr;

		MonoType* monoType = mono_reflection_type_get_type(scriptType);
		if (!monoType)
			return nullptr;

		// Get the class name from the MonoType
		MonoClass* klass = mono_type_get_class(monoType);
		if (!klass)
			return nullptr;

		const char* className = mono_class_get_name(klass);
		const char* namespaceName = mono_class_get_namespace(klass);

		// Build full class name
		std::string fullClassName = std::string(namespaceName) + "." + std::string(className);

		// Get all script instances for this entity
		auto* scriptInstances = ScriptEngine::GetEntityScriptInstances(entityID);
		if (!scriptInstances)
			return nullptr;

		// Find the script instance that matches the requested type
		for (auto& instance : *scriptInstances)
		{
			if (instance->GetScriptClass()->GetFullClassName() == fullClassName)
			{
				return instance->GetMonoObject();
			}
		}

		return nullptr;
	}
	#pragma endregion

	#pragma region Debug

	static void Debug_Log(MonoString* message)
	{
		if (!message)
			return;

		char* cMessage = mono_string_to_utf8(message);
		if (!cMessage)
			return;

		std::string strMessage(cMessage);
		mono_free(cMessage);

		Console::Log(strMessage);
	}

	static void Debug_LogWarning(MonoString* message)
	{
		if (!message)
			return;

		char* cMessage = mono_string_to_utf8(message);
		if (!cMessage)
			return;

		std::string strMessage(cMessage);
		mono_free(cMessage);

		Console::LogWarning(strMessage);
	}

	static void Debug_LogError(MonoString* message)
	{
		if (!message)
			return;

		char* cMessage = mono_string_to_utf8(message);
		if (!cMessage)
			return;

		std::string strMessage(cMessage);
		mono_free(cMessage);

		Console::LogError(strMessage);
	}

	#pragma endregion

	#pragma region TransformComponent
	static void TransformComponent_GetPosition(UUID entityID, glm::vec3* outPosition)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<TransformComponent>(), "Entity does not have TransformComponent!");
		*outPosition = entity.GetComponent<TransformComponent>().Position;
	}

	static void TransformComponent_SetPosition(UUID entityID, glm::vec3* position)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<TransformComponent>(), "Entity does not have TransformComponent!");
		entity.GetComponent<TransformComponent>().Position = *position;
	}

	static void TransformComponent_GetRotation(UUID entityID, glm::vec3* outRotation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<TransformComponent>(), "Entity does not have TransformComponent!");
		*outRotation = entity.GetComponent<TransformComponent>().Rotation;
	}

	static void TransformComponent_SetRotation(UUID entityID, glm::vec3* rotation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<TransformComponent>(), "Entity does not have TransformComponent!");
		entity.GetComponent<TransformComponent>().Rotation = *rotation;
	}

	static void TransformComponent_GetScale(UUID entityID, glm::vec3* outScale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<TransformComponent>(), "Entity does not have TransformComponent!");
		*outScale = entity.GetComponent<TransformComponent>().Scale;
	}

	static void TransformComponent_SetScale(UUID entityID, glm::vec3* scale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<TransformComponent>(), "Entity does not have TransformComponent!");
		entity.GetComponent<TransformComponent>().Scale = *scale;
	}
	#pragma endregion

	#pragma region Rigidbody2DComponent

	static void Rigidbody2DComponent_ApplyLinearImpulse(UUID entityID, glm::vec2* impulse, glm::vec2* point, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<Rigidbody2DComponent>(), "Entity does not have Rigidbody2DComponent!");

		Rigidbody2DComponent& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		scene->GetPhysicsWorld2D()->ApplyLinearImpulse(rb2d.RuntimeBody, *impulse, *point, wake);
	}

	static void Rigidbody2DComponent_ApplyLinearImpulseToCenter(UUID entityID, glm::vec2* impulse, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<Rigidbody2DComponent>(), "Entity does not have Rigidbody2DComponent!");

		Rigidbody2DComponent& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		scene->GetPhysicsWorld2D()->ApplyLinearImpulseToCenter(rb2d.RuntimeBody, *impulse, wake);
	}

	static void Rigidbody2DComponent_ApplyForce(UUID entityID, glm::vec2* force, glm::vec2* point, bool wake) 
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<Rigidbody2DComponent>(), "Entity does not have Rigidbody2DComponent!");

		Rigidbody2DComponent& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		scene->GetPhysicsWorld2D()->ApplyForce(rb2d.RuntimeBody, *force, *point, wake);
	}

	static void Rigidbody2DComponent_ApplyForceToCenter(UUID entityID, glm::vec2* force, glm::vec2* point, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		GX_ASSERT(entity.HasComponent<Rigidbody2DComponent>(), "Entity does not have Rigidbody2DComponent!");

		Rigidbody2DComponent& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		scene->GetPhysicsWorld2D()->ApplyForceToCenter(rb2d.RuntimeBody, *force, wake);
	}

	#pragma endregion

	void ScriptGlue::RegisterComponents()
	{
		// Clear previous registrations
		s_MonoTypeToTypeIndex.clear();

		// Loop through all registered components in the ComponentRegistry
		const auto& components = ComponentRegistry::Get().GetAllComponents();
		for (const auto& [typeIndex, componentInfo] : components)
		{
			// Build the C# type name: "GravixEngine.{ComponentName}Component"
			// Remove " Renderer" suffix if present (e.g., "Sprite Renderer" -> "SpriteRenderer")
			std::string componentName = componentInfo.Name;

			// Remove spaces
			componentName.erase(std::remove(componentName.begin(), componentName.end(), ' '), componentName.end());

			// Build full qualified name
			std::string fullName = "GravixEngine." + componentName + "Component";
			if (componentName == "ComponentOrder")
				continue; // Skip ComponentOrder as it does not have a C# counterpart

			// Try to get the Mono type
			MonoType* managedType = mono_reflection_type_from_name((char*)fullName.c_str(), ScriptEngine::GetCoreAssemblyImage());

			if (managedType)
			{
				// Map MonoType to std::type_index
				s_MonoTypeToTypeIndex.emplace(managedType, typeIndex);
				GX_CORE_INFO("Registered component for scripting: {0} -> {1}", fullName, componentInfo.Name);
			}
			else
			{
				GX_CORE_WARN("Failed to find C# type for component: {0} (tried: {1})", componentInfo.Name, fullName);
			}
		}
	}

	void ScriptGlue::RegisterFunctions()
	{
		RegisterComponents();

		GX_ADD_INTERNAL_CALL(Entity_HasComponent);
		GX_ADD_INTERNAL_CALL(Entity_AddComponent);
		GX_ADD_INTERNAL_CALL(Entity_RemoveComponent);
		GX_ADD_INTERNAL_CALL(Entity_FindEntityByName);
		GX_ADD_INTERNAL_CALL(Entity_GetScriptInstance);

		GX_ADD_INTERNAL_CALL(TransformComponent_GetPosition);
		GX_ADD_INTERNAL_CALL(TransformComponent_SetPosition);
		GX_ADD_INTERNAL_CALL(TransformComponent_GetRotation);
		GX_ADD_INTERNAL_CALL(TransformComponent_SetRotation);
		GX_ADD_INTERNAL_CALL(TransformComponent_GetScale);
		GX_ADD_INTERNAL_CALL(TransformComponent_SetScale);

		GX_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulse);
		GX_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulseToCenter);
		GX_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyForce);
		GX_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyForceToCenter);

		GX_ADD_INTERNAL_CALL(Input_IsKeyDown);
		GX_ADD_INTERNAL_CALL(Input_IsKeyPressed);

		GX_ADD_INTERNAL_CALL(Debug_Log);
		GX_ADD_INTERNAL_CALL(Debug_LogWarning);
		GX_ADD_INTERNAL_CALL(Debug_LogError);
	}
}