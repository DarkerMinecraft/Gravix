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
			DrawAddComponents(selectedEntity);
		}
		ImGui::End();
	}

	void InspectorPanel::DrawComponents(Entity entity)
	{
		for (auto typeIndex : ComponentRegistry::Get().GetComponentOrder())
		{
			const auto& info = ComponentRegistry::Get().GetAllComponents().at(typeIndex);
			if (info.ImGuiRenderFunc && info.GetComponentFunc)
			{
				void* component = info.GetComponentFunc(m_SceneHierarchyPanel->GetContext()->m_Registry, entity);
				if (component)
				{
					info.ImGuiRenderFunc(component);
				}
			}
		}
	}

	void InspectorPanel::DrawAddComponents(Entity entity)
	{
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			for (auto typeIndex : ComponentRegistry::Get().GetComponentOrder())
			{
				const auto& info = ComponentRegistry::Get().GetAllComponents().at(typeIndex);


			}
			ImGui::EndPopup();
		}
	}

}