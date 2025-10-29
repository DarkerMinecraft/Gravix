#include "InspectorPanel.h"
#include <imgui.h>

#include "Scene/Entity.h"

namespace Gravix
{

	InspectorPanel::InspectorPanel(SceneHierarchyPanel sceneHierarchyPanel)
	{
		SetSceneHierarchyPanel(sceneHierarchyPanel);
	}

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Inspector");
		Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
		if (selectedEntity)
		{
			DrawComponents(selectedEntity);
		}
		ImGui::End();
	}

	void InspectorPanel::DrawComponents(Entity entity)
	{
		for (auto& componentType : entity.GetAddedComponents())
		{
			if (auto* info = ComponentRegistry::Get().GetComponentInfo(componentType))
			{
				if (info->ImGuiRenderFunc && info->GetComponentFunc)
				{
					// Use the GetComponentFunc to retrieve the component at runtime
					void* component = info->GetComponentFunc(entity.GetRegistry(), entity.GetHandle());
					if (component)
					{
						info->ImGuiRenderFunc(component);
					}
				}
			}
		}
	}

}