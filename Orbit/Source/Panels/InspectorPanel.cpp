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
			if (info.ImGuiRenderFunc)
			{
				if(!entity.HasComponent(typeIndex))
					continue;

				void* component = entity.GetComponent(typeIndex);
				if (component)
				{
					ComponentUserSettings userSettings;
					info.ImGuiRenderFunc(component, &userSettings);

					if(userSettings.RemoveComponent && info.RemoveComponentFunc)
					{
						entity.RemoveComponent(typeIndex);
					}
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

				if(!info.HasComponentFunc || !info.AddComponentFunc)
					continue;

				bool hasComponent = entity.HasComponent(typeIndex);
				if (!hasComponent)
				{
					if(info.Name.empty())
						continue;

					if (ImGui::MenuItem(info.Name.c_str()))
					{
						entity.AddComponent(typeIndex);
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::EndPopup();
		}
	}

}