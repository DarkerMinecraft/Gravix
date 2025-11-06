#include "ViewportPanel.h"

#include "Maths/Maths.h"
#include "Core/Application.h"

#include "Input.h"

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
		auto viewportOffset = ImGui::GetCursorPos();

		// Track if the viewport is hovered and focused
		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();
		Application::Get().GetImGui().BlockEvents(!m_ViewportHovered || m_ViewportFocused);

		ImVec2 avail = ImGui::GetContentRegionAvail();
		m_ViewportSize = { avail.x, avail.y };
		ImGui::Image(m_Framebuffer->GetColorAttachmentID(m_RenderIndex), avail);

		auto windowSize = ImGui::GetWindowSize();
		ImVec2 minBound = ImGui::GetWindowPos();
		minBound.x += viewportOffset.x;
		minBound.y += viewportOffset.y;

		ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.x };
		m_ViewportBounds[0] = { minBound.x, minBound.y };
		m_ViewportBounds[1] = { maxBound.x, maxBound.y };

		Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
		if (selectedEntity && m_GuizmoType != -1) 
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

			bool snap = Input::IsKeyDown(Key::LeftControl);
			float snapValue = m_GuizmoType == ImGuizmo::OPERATION::ROTATE ? 45.0f : 0.5f;

			float snapValues[3] = { snapValue, snapValue, snapValue };

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), (ImGuizmo::OPERATION)m_GuizmoType,
				ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr, snap ? snapValues : nullptr);

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

	void ViewportPanel::UpdateViewport()
	{
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[1].y;

		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		int mouseX = (int)mx;
		int mouseY = (int)my;

		int pixel = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
		GX_CORE_INFO("Pixel Data: {0}", pixel);
	}

	void ViewportPanel::GuizmoShortcuts()
	{
		if(Input::IsKeyPressed(Key::Q))
			m_GuizmoType = -1;

		if(Input::IsKeyPressed(Key::W))
			m_GuizmoType = ImGuizmo::OPERATION::TRANSLATE;
		if (Input::IsKeyPressed(Key::E))
			m_GuizmoType = ImGuizmo::OPERATION::ROTATE;
		if (Input::IsKeyPressed(Key::R))
			m_GuizmoType = ImGuizmo::OPERATION::SCALE;
	}

}