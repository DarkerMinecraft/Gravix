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
		bool hasOrderComponent = false;

		if (entity.HasComponent<ComponentOrderComponent>())
		{
			auto& orderComponent = entity.GetComponent<ComponentOrderComponent>();
			componentOrder = orderComponent.ComponentOrder;
			hasOrderComponent = true;
		}
		else
		{
			// Fallback to registry order if ComponentOrderComponent doesn't exist
			componentOrder = ComponentRegistry::Get().GetComponentOrder();
		}

		int componentIndex = 0;
		int draggedComponentIndex = -1;
		int targetComponentIndex = -1;

		for (auto typeIndex : componentOrder)
		{
			const auto& allComponents = ComponentRegistry::Get().GetAllComponents();
			auto it = allComponents.find(typeIndex);
			if (it == allComponents.end())
			{
				componentIndex++;
				continue;
			}

			const auto& info = it->second;
			if (info.ImGuiRenderFunc)
			{
				// Check if this component allows multiple instances
				if (info.Specification.AllowMultiple)
				{
					// Handle multi-instance components
					Scene* scene = entity.GetScene();
					UUID entityID = entity.GetID();

					auto entityIt = scene->m_MultiComponents.find(entityID);
					if (entityIt != scene->m_MultiComponents.end())
					{
						auto compIt = entityIt->second.find(typeIndex);
						if (compIt != entityIt->second.end())
						{
							auto& instances = compIt->second;

							// Render each instance
							for (size_t i = 0; i < instances.size(); i++)
							{
								void* component = instances[i].get();

								// Store cursor position before rendering component
								ImVec2 cursorPosBefore = ImGui::GetCursorPos();

								ComponentUserSettings userSettings;
								info.ImGuiRenderFunc(component, &userSettings);

								// Get the last item rect for drag and drop
								ImVec2 cursorPosAfter = ImGui::GetCursorPos();

								// Mark scene dirty if component was modified
								if (userSettings.WasModified)
								{
									if (m_AppLayer)
										m_AppLayer->MarkSceneDirty();
								}

								if(userSettings.RemoveComponent)
								{
									// Remove this specific instance
									instances.erase(instances.begin() + i);
									if (m_AppLayer)
										m_AppLayer->MarkSceneDirty();
									break; // Exit loop after removal to avoid iterator issues
								}
							}

							// Add button to create new instances
							ImGui::Spacing();
							std::string addButtonLabel = "+ Add " + info.Name;
							if (ImGui::Button(addButtonLabel.c_str()))
							{
								// Add new instance using AddComponentInstance
								// For ScriptComponent specifically
								if (typeIndex == typeid(ScriptComponent))
								{
									entity.AddComponentInstance<ScriptComponent>();
									if (m_AppLayer)
										m_AppLayer->MarkSceneDirty();
								}
							}
							ImGui::Spacing();
						}
					}
				}
				else
				{
					// Handle single-instance components
					if(!entity.HasComponent(typeIndex))
					{
						componentIndex++;
						continue;
					}

					void* component = entity.GetComponent(typeIndex);
					if (component)
					{
						// Store cursor position before rendering component
						ImVec2 cursorPosBefore = ImGui::GetCursorPos();

						ComponentUserSettings userSettings;
						info.ImGuiRenderFunc(component, &userSettings);

						// Get the last item rect for drag and drop
						ImVec2 cursorPosAfter = ImGui::GetCursorPos();
						ImVec2 itemMin = ImVec2(cursorPosBefore.x, cursorPosBefore.y);
						ImVec2 itemMax = ImVec2(cursorPosAfter.x, cursorPosAfter.y);

						// Make the entire component area draggable
						ImGui::SetCursorPos(cursorPosBefore);
						ImGui::InvisibleButton(("##component_drag_" + std::to_string(componentIndex)).c_str(),
							ImVec2(ImGui::GetContentRegionAvail().x, cursorPosAfter.y - cursorPosBefore.y));
						ImGui::SetCursorPos(cursorPosAfter);

						// Drag and drop source
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
						{
							ImGui::SetDragDropPayload("COMPONENT_REORDER", &componentIndex, sizeof(int));
							ImGui::Text("Reorder: %s", info.Name.c_str());
							ImGui::EndDragDropSource();
						}

						// Drag and drop target
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT_REORDER"))
							{
								draggedComponentIndex = *(int*)payload->Data;
								targetComponentIndex = componentIndex;
							}
							ImGui::EndDragDropTarget();
						}

						// Mark scene dirty if component was modified
						if (userSettings.WasModified)
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

			componentIndex++;
		}

		// Apply component reordering if drag and drop occurred
		if (draggedComponentIndex != -1 && targetComponentIndex != -1 && draggedComponentIndex != targetComponentIndex)
		{
			// Ensure entity has ComponentOrderComponent
			if (!hasOrderComponent)
			{
				entity.AddComponent<ComponentOrderComponent>();
				entity.GetComponent<ComponentOrderComponent>().ComponentOrder = componentOrder;
			}

			// Reorder components
			auto& orderComponent = entity.GetComponent<ComponentOrderComponent>();
			std::type_index draggedType = orderComponent.ComponentOrder[draggedComponentIndex];
			orderComponent.ComponentOrder.erase(orderComponent.ComponentOrder.begin() + draggedComponentIndex);
			orderComponent.ComponentOrder.insert(orderComponent.ComponentOrder.begin() + targetComponentIndex, draggedType);

			if (m_AppLayer)
				m_AppLayer->MarkSceneDirty();
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

				// Allow multi-instance components to be added multiple times
				if (!hasComponent || info.Specification.AllowMultiple)
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
						// For multi-instance components, use AddComponentInstance
						if (info.Specification.AllowMultiple)
						{
							if (typeIndex == typeid(ScriptComponent))
							{
								entity.AddComponentInstance<ScriptComponent>();
							}
						}
						else
						{
							entity.AddComponent(typeIndex);
						}

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