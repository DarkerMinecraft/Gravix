#include "ViewportPanel.h"

#include "Maths/Maths.h"
#include "Core/Application.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>

namespace Gravix
{
	ViewportPanel::ViewportPanel(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex)
	{
		SetFramebuffer(framebuffer, renderIndex);
	}

	void ViewportPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");

		// Track if the viewport is hovered and focused
		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();
		Application::Get().GetImGui().BlockEvents(!m_ViewportHovered || m_ViewportFocused);

		ImVec2 avail = ImGui::GetContentRegionAvail();
		m_ViewportSize = { avail.x, avail.y };

		ImGui::Image(m_Framebuffer->GetColorAttachmentID(m_RenderIndex), avail);
		Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
		m_GuizmoType = ImGuizmo::OPERATION::ROTATE;
		if (selectedEntity) 
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			float windowWidth = (float)ImGui::GetWindowWidth();
			float windowHeight = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

			glm::mat4 cameraProjection = m_EditorCamera->GetProjection();
			cameraProjection[1][1] *= -1; 
			const glm::mat4& cameraView = m_EditorCamera->GetViewMatrix();
			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.Transform;

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), (ImGuizmo::OPERATION)m_GuizmoType,
				ImGuizmo::LOCAL, glm::value_ptr(transform));

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 position, rotation, scale;
				Math::DecomposeTransform(transform, position, rotation, scale);

				glm::vec3 deltaRotation = rotation - tc.Rotation;
				tc.Position = position;
				tc.Rotation += deltaRotation;
				tc.Scale = scale;

				tc.CalculateTransform();
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}