#include "InspectorPanel.h"
#include <imgui.h>

#include "Scene/Entity.h"

namespace Gravix
{

	InspectorPanel::InspectorPanel(SceneHierarchyPanel* sceneHierarchyPanel)
	{
		SetSceneHierarchyPanel(sceneHierarchyPanel);
	}

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Inspector");
		Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
		if (selectedEntity)
		{
			DrawComponents(selectedEntity);
		}
		ImGui::End();
	}

	void InspectorPanel::DrawComponents(Entity entity)
	{
		for (const auto& [typeIndex, info] : ComponentRegistry::Get().GetAllComponents())
		{
			if (info.ImGuiRenderFunc && info.GetComponentFunc)
			{
				void* component = info.GetComponentFunc(entity.GetRegistry(), entity.GetHandle());
				if (component)
				{
					info.ImGuiRenderFunc(component);
				}
			}
		}
	}

}