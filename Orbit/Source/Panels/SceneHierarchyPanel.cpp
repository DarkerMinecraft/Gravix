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

		if (ImGui::BeginPopupContextWindow("EntityCreation"))
		{
			if (ImGui::MenuItem("Create Entity"))
			{
				m_Context->CreateEntity("Entity");
			}
			ImGui::EndPopup();
		}

		for (auto entityID : m_Context->m_Registry.view<entt::entity>()) 
		{
			Entity entity{ entityID, m_Context.get() };
			DrawEntityNode(entity);
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& name = entity.GetName();

		ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, name.c_str());
		if(ImGui::IsItemClicked())
		{
			m_SelectedEntity = entity;
		}

		if (opened) 
		{
			ImGui::TreePop();
		}
	}

}