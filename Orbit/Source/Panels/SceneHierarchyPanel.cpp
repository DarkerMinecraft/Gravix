#include "SceneHierarchyPanel.h"
#include "AppLayer.h"

#include <imgui.h>
#include <vector>
#include <algorithm>

#include "Scene/Components.h"

namespace Gravix
{

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene)
	{
		SetContext(scene);		
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;

		m_SelectedEntity = { entt::null, m_Context.get() };
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		if (m_Context == nullptr) return;

		ImGui::Begin("Scene Hierarchy");

		// Unity-style hierarchy rendering with entities sorted by creation order
		std::vector<Entity> sortedEntities;
		for (auto entityID : m_Context->m_Registry.view<TagComponent>())
		{
			Entity entity{ entityID, m_Context.get() };
			if(entity)
				sortedEntities.push_back(entity);
		}

		// Sort entities by creation index to maintain insertion order
		std::sort(sortedEntities.begin(), sortedEntities.end(),
			[](const Entity& a, const Entity& b)
			{
				return a.GetComponent<TagComponent>().CreationIndex < b.GetComponent<TagComponent>().CreationIndex;
			});

		// Draw entities in sorted order
		for (auto& entity : sortedEntities)
		{
			if(entity)
				DrawEntityNode(entity);
		}

		// Deselect when clicking empty space (using proper enum instead of magic number)
		if(ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
		{
			m_SelectedEntity = { entt::null, m_Context.get() };
		}

		// Unity-style right-click context menu
		ImGuiPopupFlags popupFlags = ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems;
		if (ImGui::BeginPopupContextWindow(0, popupFlags))
		{
			if (ImGui::MenuItem("Create Empty Entity"))
			{
				m_Context->CreateEntity("Entity");
				if (m_AppLayer) m_AppLayer->MarkSceneDirty();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Create Sprite"))
			{
				Entity newEntity = m_Context->CreateEntity("Sprite");
				newEntity.AddComponent<SpriteRendererComponent>();
				if (m_AppLayer) m_AppLayer->MarkSceneDirty();
			}

			if (ImGui::MenuItem("Create Camera"))
			{
				Entity newEntity = m_Context->CreateEntity("Camera");
				newEntity.AddComponent<CameraComponent>();
				if (m_AppLayer) m_AppLayer->MarkSceneDirty();
			}

			ImGui::EndPopup();
		}

		// Keyboard shortcuts when window is focused
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		{
			// Ctrl+D to duplicate selected entity
			if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
			{
				if (m_SelectedEntity)
				{
					m_Context->DuplicateEntity(m_SelectedEntity);
					if (m_AppLayer) m_AppLayer->MarkSceneDirty();
				}
			}
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& name = entity.GetName();

		// Unity-style tree node flags
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_FramePadding;

		if (m_SelectedEntity && m_SelectedEntity == entity)
			flags |= ImGuiTreeNodeFlags_Selected;

		// Add leaf flag since entities don't have children yet
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, name.c_str());

		if(ImGui::IsItemClicked())
		{
			m_SelectedEntity = entity;
		}

		// Drag and drop source
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload("ENTITY_HIERARCHY", &entity, sizeof(Entity));
			ImGui::Text("Move: %s", name.c_str());
			ImGui::EndDragDropSource();
		}

		// Drag and drop target
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_HIERARCHY"))
			{
				Entity draggedEntity = *(Entity*)payload->Data;

				// Swap creation indices to reorder entities
				if (draggedEntity && entity && draggedEntity != entity)
				{
					auto& draggedTag = draggedEntity.GetComponent<TagComponent>();
					auto& targetTag = entity.GetComponent<TagComponent>();

					uint32_t tempIndex = draggedTag.CreationIndex;
					draggedTag.CreationIndex = targetTag.CreationIndex;
					targetTag.CreationIndex = tempIndex;

					if (m_AppLayer) m_AppLayer->MarkSceneDirty();
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Unity-style context menu
		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Duplicate"))
			{
				if (m_SelectedEntity)
				{
					m_Context->DuplicateEntity(m_SelectedEntity);
					if (m_AppLayer) m_AppLayer->MarkSceneDirty();
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Delete"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (entityDeleted)
		{
			if (m_SelectedEntity && m_SelectedEntity == entity)
				m_SelectedEntity = { entt::null, m_Context.get() };
			m_Context->DestroyEntity(entity);
			if (m_AppLayer) m_AppLayer->MarkSceneDirty();
		}
	}

}