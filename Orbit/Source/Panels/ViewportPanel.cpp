#include "ViewportPanel.h"

#include "Maths/Maths.h"
#include "Core/Application.h"
#include "AppLayer.h"

#include "Input.h"
#include "Events/KeyEvents.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>


namespace Gravix
{
	ViewportPanel::ViewportPanel(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex)
	{
		SetFramebuffer(framebuffer, renderIndex);
	}

	void ViewportPanel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(ViewportPanel::OnKeyPressed));
	}

	bool ViewportPanel::OnKeyPressed(KeyPressedEvent& e)
	{
		// Only process shortcuts if viewport is hovered
		if (!m_ViewportHovered)
			return false;

		// Don't process shortcuts if ImGui wants keyboard input
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureKeyboard && (io.WantTextInput || ImGui::IsAnyItemActive()))
			return false;

		// Guizmo shortcuts
		switch (e.GetKeyCode())
		{
		case Key::Q:
			m_GuizmoType = -1;
			return true;

		case Key::W:
			m_GuizmoType = ImGuizmo::OPERATION::TRANSLATE;
			return true;

		case Key::E:
			m_GuizmoType = ImGuizmo::OPERATION::ROTATE;
			return true;

		case Key::R:
			m_GuizmoType = ImGuizmo::OPERATION::SCALE;
			return true;
		}

		return false;
	}

	void ViewportPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();

		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		// Track if the viewport is hovered and focused
		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();
		Application::Get().GetImGui().BlockEvents(!m_ViewportHovered || m_ViewportFocused);

		ImVec2 avail = ImGui::GetContentRegionAvail();
		m_ViewportSize = { avail.x, avail.y };
		ImGui::Image(m_Framebuffer->GetColorAttachmentID(m_RenderIndex), avail, ImVec2(0, 1), ImVec2(1, 0));

		if (ImGui::BeginDragDropTarget()) 
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) 
			{
				AssetHandle sceneHandle = *(AssetHandle*)payload->Data;
				m_AppLayer->OpenScene(sceneHandle);
			}
			ImGui::EndDragDropTarget();
		}

		Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
		if (selectedEntity && m_GuizmoType != -1) 
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			float windowWidth = (float)ImGui::GetWindowWidth();
			float windowHeight = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

			glm::mat4 cameraProjection = m_EditorCamera->GetProjection();
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
		if (m_ViewportBounds.empty())
			return;

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;

		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
		{
			int pixel = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
			m_HoveredEntity = pixel == -1 ? Entity{ entt::null, m_SceneHierarchyPanel->GetContext().get() } : Entity((entt::entity)(uint64_t)(uint32_t)pixel, m_SceneHierarchyPanel->GetContext().get());

			// Only change cursor mode if it actually needs to change (prevents glitching)
			CursorMode desiredMode = (pixel != -1) ? CursorMode::Pointer : CursorMode::Normal;
			if (m_CurrentCursorMode != desiredMode)
			{
				m_CurrentCursorMode = desiredMode;
				Application::Get().GetWindow().SetCursorMode(desiredMode);
			}
		}
	}

}