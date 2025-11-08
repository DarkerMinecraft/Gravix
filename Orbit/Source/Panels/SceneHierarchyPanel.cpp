#include "SceneHierarchyPanel.h"
#include <imgui.h>

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
		ImGui::Begin("Scene Hierarchy");

		// Unity-style hierarchy rendering
		for (auto entityID : m_Context->m_Registry.view<TagComponent>())
		{
			Entity entity{ entityID, m_Context.get() };

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
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Create Sprite"))
			{
				Entity newEntity = m_Context->CreateEntity("Sprite");
				newEntity.AddComponent<SpriteRendererComponent>();
			}

			if (ImGui::MenuItem("Create Camera"))
			{
				Entity newEntity = m_Context->CreateEntity("Camera");
				newEntity.AddComponent<CameraComponent>();
			}

			ImGui::EndPopup();
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

		// Unity-style context menu
		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Duplicate"))
			{
				// TODO: Implement entity duplication
			}

			if (ImGui::MenuItem("Rename"))
			{
				// TODO: Implement entity renaming
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Delete", "Delete"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (entityDeleted)
		{
			if (m_SelectedEntity && m_SelectedEntity == entity)
				m_SelectedEntity = { entt::null, m_Context.get() };
			m_Context->DestroyEntity(entity);
		}
	}

}