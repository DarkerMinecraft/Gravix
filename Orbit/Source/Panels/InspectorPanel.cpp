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
		if(selectedEntity)
		{
			DrawComponents(selectedEntity);
		}
		ImGui::End();
	}

	void InspectorPanel::DrawComponents(Entity entity)
	{
		for(auto& componentType : entity.GetAddedComponents())
		{
			if(auto* info = ComponentRegistry::Get().GetComponentInfo(componentType))
			{
				if(info->ImGuiRenderFunc)
				{
					auto component = entity.GetComponent<std::remove_pointer_t<decltype(componentType)>>();
					info->ImGuiRenderFunc(&component);
				}
			}
		}
	}

}