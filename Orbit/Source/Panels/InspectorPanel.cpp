#include "InspectorPanel.h"
#include "AppLayer.h"

#include <imgui.h>
#include <algorithm>
#include <vector>
#include <typeindex>

#include "Scene/Entity.h"
#include "Scene/Components.h"

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

			// Unity-style separator before "Add Component" button
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			DrawAddComponents(selectedEntity);
		}
		else
		{
			// Unity-style centered message when no entity is selected
			ImVec2 windowSize = ImGui::GetWindowSize();
			ImVec2 textSize = ImGui::CalcTextSize("No Entity Selected");
			ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
			ImGui::SetCursorPosY(windowSize.y * 0.5f);
			ImGui::TextDisabled("No Entity Selected");
		}

		ImGui::End();
	}

	void InspectorPanel::DrawComponents(Entity entity)
	{
		// Use ComponentOrderComponent to determine rendering order if it exists
		std::vector<std::type_index> componentOrder;

		if (entity.HasComponent<ComponentOrderComponent>())
		{
			const auto& orderComponent = entity.GetComponent<ComponentOrderComponent>();
			componentOrder = orderComponent.ComponentOrder;
		}
		else
		{
			// Fallback to registry order if ComponentOrderComponent doesn't exist
			componentOrder = ComponentRegistry::Get().GetComponentOrder();
		}

		for (auto typeIndex : componentOrder)
		{
			const auto& allComponents = ComponentRegistry::Get().GetAllComponents();
			auto it = allComponents.find(typeIndex);
			if (it == allComponents.end())
				continue;

			const auto& info = it->second;
			if (info.ImGuiRenderFunc)
			{
				if(!entity.HasComponent(typeIndex))
					continue;

				void* component = entity.GetComponent(typeIndex);
				if (component)
				{
					ComponentUserSettings userSettings;
					info.ImGuiRenderFunc(component, &userSettings);

					// Check if any component value was modified
					if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemEdited())
					{
						if (m_AppLayer)
							m_AppLayer->MarkSceneDirty();
					}

					if(userSettings.RemoveComponent && info.RemoveComponentFunc)
					{
						entity.RemoveComponent(typeIndex);
						if (m_AppLayer)
							m_AppLayer->MarkSceneDirty();
					}
				}
			}
		}
	}

	void InspectorPanel::DrawAddComponents(Entity entity)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Unity-style "Add Component" button - full width with bold text
		float buttonWidth = ImGui::GetContentRegionAvail().x;
		ImVec2 buttonSize = ImVec2(buttonWidth, 0.0f);

		// Style the button with Unity colors
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.267f, 0.267f, 0.267f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.349f, 0.349f, 0.349f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.443f, 0.443f, 0.443f, 1.0f));
		ImGui::PushFont(io.Fonts->Fonts[1]); // Bold font

		if (ImGui::Button("Add Component", buttonSize))
			ImGui::OpenPopup("AddComponent");

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		if (ImGui::BeginPopup("AddComponent"))
		{
			// Bold header
			ImGui::PushFont(io.Fonts->Fonts[1]);
			ImGui::TextColored(ImVec4(0.267f, 0.529f, 0.808f, 1.0f), "Add Component");
			ImGui::PopFont();
			ImGui::Separator();

			// Search filter for components (Unity-style)
			static char searchBuffer[256] = "";
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##ComponentSearch", "Search...", searchBuffer, sizeof(searchBuffer));

			ImGui::Separator();

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

					// Filter by search text
					if (searchBuffer[0] != '\0')
					{
						std::string nameLower = info.Name;
						std::string searchLower = searchBuffer;
						std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
						std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

						if (nameLower.find(searchLower) == std::string::npos)
							continue;
					}

					if (ImGui::MenuItem(info.Name.c_str()))
					{
						entity.AddComponent(typeIndex);
						if (m_AppLayer)
							m_AppLayer->MarkSceneDirty();
						searchBuffer[0] = '\0'; // Clear search on selection
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::EndPopup();
		}
	}

}