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

		for (auto entityID : m_Context->m_Registry.view<TagComponent>()) 
		{
			Entity entity{ entityID, m_Context.get() };

			if(entity)
				DrawEntityNode(entity);
		}

		if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
		{
			m_SelectedEntity = { entt::null, m_Context.get() };
		}

		ImGuiPopupFlags popupFlags = ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems;
		if (ImGui::BeginPopupContextWindow(0, popupFlags))
		{
			if (ImGui::MenuItem("Create Entity"))
			{
				m_Context->CreateEntity("Entity");
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& name = entity.GetName();

		ImGuiTreeNodeFlags flags = (m_SelectedEntity && m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, name.c_str());
		if(ImGui::IsItemClicked())
		{
			m_SelectedEntity = entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem()) 
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (opened) 
		{
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			if (m_SelectedEntity && m_SelectedEntity == entity)
				m_SelectedEntity = { entt::null, m_Context.get() };
			m_Context->DestroyEntity(entity);
		}
	}

}