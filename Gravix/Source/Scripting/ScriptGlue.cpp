#include "pch.h"
#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include <mono/jit/jit.h>

#include "Core/UUID.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"

#define GX_ADD_INTERNAL_CALL(name) mono_add_internal_call("GravixEngine.InternalCalls::"#name, name) 
namespace Gravix 
{

	static void Entity_GetPosition(UUID entityID, glm::vec3* outPosition) 
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		if (entity && entity.HasComponent<TransformComponent>()) 
		{
			*outPosition = entity.GetComponent<TransformComponent>().Position;
		}
	}

	static void Entity_SetPosition(UUID entityID, glm::vec3* position) 
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);
		if (entity && entity.HasComponent<TransformComponent>()) 
		{
			auto& transform = entity.GetComponent<TransformComponent>();
			transform.Position = *position;
			transform.CalculateTransform();
		}
	}

	void ScriptGlue::RegisterFunctions()
	{
		GX_ADD_INTERNAL_CALL(Entity_GetPosition);
		GX_ADD_INTERNAL_CALL(Entity_SetPosition);
	}
}