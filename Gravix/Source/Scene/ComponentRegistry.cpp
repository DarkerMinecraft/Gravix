#include "pch.h"
#include "ComponentRegistry.h"

#include "Components.h"

#include <imgui.h>

namespace Gravix 
{

	void ComponentRegistry::RegisterAllComponents()
	{
		RegisterComponent<TagComponent>(
			"TagComponent",
			[](TagComponent& c, Scene* scene)
			{
			},
			[](TagComponent& c)
			{
				ImGui::Text("Name: ");
				ImGui::SameLine();
				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, c.Name.c_str());
				if (ImGui::InputText("##TagComponentName", buffer, sizeof(buffer)))
				{
					c.Name = std::string(buffer);
				}
			}
		);

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