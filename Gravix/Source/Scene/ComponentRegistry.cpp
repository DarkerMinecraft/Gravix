#include "pch.h"
#include "ComponentRegistry.h"

#include "Components.h"

#include <imgui.h>

namespace Gravix 
{

	void ComponentRegistry::RegisterAllComponents()
	{
		RegisterComponent<TransformComponent>(
			"TransformComponent",
			nullptr,
			[](TransformComponent& c)
			{

			}
		);

		RegisterComponent<CameraComponent>(
			"CameraCompoent",
			[](CameraComponent& c, Scene* scene) 
			{

			},
			[](CameraComponent& c) 
			{

			}
		);
	}

}