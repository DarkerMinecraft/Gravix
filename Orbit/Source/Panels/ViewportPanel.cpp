#include "ViewportPanel.h"

#include "Maths/Maths.h"
#include "Core/Application.h"
#include "AppLayer.h"
#include "SceneManager.h"
#include "Asset/Importers/TextureImporter.h"

#include "Core/Input.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>


namespace Gravix
{
	ViewportPanel::ViewportPanel(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex)
	{
		SetFramebuffer(framebuffer, renderIndex);

		// Load toolbar icons
		m_IconPlay = TextureImporter::LoadTexture2D("EditorAssets/Icons/PlayButton.png");
		m_IconStop = TextureImporter::LoadTexture2D("EditorAssets/Icons/StopButton.png");
	}

	void ViewportPanel::LoadIcons()
	{
		if (!m_IconPlay)
			m_IconPlay = TextureImporter::LoadTexture2D("EditorAssets/Icons/PlayButton.png");
		if (!m_IconStop)
			m_IconStop = TextureImporter::LoadTexture2D("EditorAssets/Icons/StopButton.png");
	}

	void ViewportPanel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(ViewportPanel::OnKeyPressed));
	}

	bool ViewportPanel::OnKeyPressed(KeyPressedEvent& e)
	{
		// Only process shortcuts if viewport is focused
		if (!m_ViewportFocused)
			return false;

		// Only process ImGuizmo shortcuts in Edit mode
		if (!m_SceneManager || m_SceneManager->GetSceneState() != SceneState::Edit)
			return false;

		// Don't process shortcuts if ImGui wants keyboard input
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureKeyboard && (io.WantTextInput || ImGui::IsAnyItemActive()))
			return false;

		// Guizmo shortcuts (only in Edit mode)
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
		if(m_Framebuffer == nullptr)
			return;

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
		Application::Get().GetImGui().BlockEvents(!m_ViewportFocused || !m_ViewportHovered);

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

		// Draw toolbar overlay at the top
		DrawToolbarOverlay();

		// Draw gizmo mode buttons in Edit mode
		if (m_SceneManager && m_SceneManager->GetSceneState() == SceneState::Edit)
		{
			DrawGizmoModeButtons();
		}

		// Only show ImGuizmo in Edit mode
		if (m_SceneManager && m_SceneManager->GetSceneState() == SceneState::Edit)
		{
			Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();

			// Auto-deselect gizmo when nothing is selected
			if (!selectedEntity)
				m_GuizmoType = -1;

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

				bool snap = (m_ViewportHovered && m_ViewportFocused) && Input::IsKeyDown(Key::LeftControl);
				float snapValue = m_GuizmoType == ImGuizmo::OPERATION::ROTATE ? 45.0f : 0.5f;

				float snapValues[3] = { snapValue, snapValue, snapValue };

				ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), (ImGuizmo::OPERATION)m_GuizmoType,
					ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr, snap ? snapValues : nullptr);

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 position, rotation, scale;
					Math::DecomposeTransform(transform, position, rotation, scale);

					// Prevent scale from going to zero or negative (causes haywire behavior)
					scale.x = glm::max(scale.x, 0.001f);
					scale.y = glm::max(scale.y, 0.001f);
					scale.z = glm::max(scale.z, 0.001f);

					glm::vec3 deltaRotation = rotation - tc.Rotation;
					tc.Position = position;
					tc.Rotation += deltaRotation;
					tc.Scale = scale;

					tc.CalculateTransform();
				}
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ViewportPanel::UpdateViewport()
	{
		// Only perform picking if viewport is valid and hovered
		if (!m_ViewportHovered || m_ViewportSize.x <= 0 || m_ViewportSize.y <= 0)
		{
			// Reset to normal cursor when not hovering
			if (m_CurrentCursorMode != CursorMode::Normal)
			{
				m_CurrentCursorMode = CursorMode::Normal;
				Application::Get().GetWindow().SetCursorMode(CursorMode::Normal);
			}
			m_HoveredEntity = Entity{ entt::null, m_SceneHierarchyPanel->GetContext().get() };
			return;
		}

		// Skip mouse picking if using ImGuizmo to avoid interfering
		if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
		{
			m_HoveredEntity = Entity{ entt::null, m_SceneHierarchyPanel->GetContext().get() };
			return;
		}

		// Get mouse position relative to viewport
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;

		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		int mouseX = (int)mx;
		int mouseY = (int)my;

		// Verify mouse is within viewport bounds
		if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
		{
			// Read entity ID from framebuffer's second attachment (index 1)
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
		else
		{
			// Reset to normal cursor when outside viewport
			if (m_CurrentCursorMode != CursorMode::Normal)
			{
				m_CurrentCursorMode = CursorMode::Normal;
				Application::Get().GetWindow().SetCursorMode(CursorMode::Normal);
			}
			m_HoveredEntity = Entity{ entt::null, m_SceneHierarchyPanel->GetContext().get() };
		}
	}

	void ViewportPanel::DrawToolbarOverlay()
	{
		if (!m_SceneManager || !m_IconPlay || !m_IconStop)
			return;

		// Center the Play/Stop button at the top (match ImGuizmo button size)
		float boxSize = 28.0f;
		float padding = 3.0f;
		float buttonSize = boxSize - padding * 2;
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		float centerX = (viewportSize.x - boxSize) * 0.5f;

		ImGui::SetCursorPos(ImVec2(centerX, 8));

		// Draw gray rounded background box
		ImVec2 boxMin = ImGui::GetCursorScreenPos();
		ImVec2 boxMax = ImVec2(boxMin.x + boxSize, boxMin.y + boxSize);
		ImGui::GetWindowDrawList()->AddRectFilled(boxMin, boxMax, IM_COL32(60, 60, 60, 200), 4.0f);

		// Position button inside the box
		ImGui::SetCursorPos(ImVec2(centerX + padding, 8 + padding));

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		auto& colors = ImGui::GetStyle().Colors;
		auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.3f));
		auto& buttonActive = colors[ImGuiCol_ButtonActive];
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

		Ref<Texture2D> icon = (m_SceneManager->GetSceneState() == SceneState::Edit) ? m_IconPlay : m_IconStop;

		if (ImGui::ImageButton("##SceneState", (ImTextureID)icon->GetImGuiAttachment(), ImVec2{ buttonSize, buttonSize }))
		{
			if (m_SceneManager->GetSceneState() == SceneState::Edit)
				m_SceneManager->Play();
			else if (m_SceneManager->GetSceneState() == SceneState::Play)
				m_SceneManager->Stop();
		}

		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(3);
	}

	void ViewportPanel::DrawGizmoModeButtons()
	{
		// Position the gizmo buttons on the left side at the top
		float buttonSize = 28.0f;
		float spacing = 2.0f;

		ImGui::SetCursorPos(ImVec2(8, 8));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

		// None/Select button (Q)
		bool isNoneSelected = (m_GuizmoType == -1);
		if (isNoneSelected)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.5f, 0.6f, 0.9f));
		if (ImGui::Button("Q", ImVec2(buttonSize, buttonSize)))
			m_GuizmoType = -1;
		if (isNoneSelected)
			ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Select Mode (Q)");

		ImGui::SameLine();

		// Translate button (W)
		bool isTranslateSelected = (m_GuizmoType == ImGuizmo::OPERATION::TRANSLATE);
		if (isTranslateSelected)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.5f, 0.6f, 0.9f));
		if (ImGui::Button("W", ImVec2(buttonSize, buttonSize)))
			m_GuizmoType = ImGuizmo::OPERATION::TRANSLATE;
		if (isTranslateSelected)
			ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Translate Mode (W)");

		ImGui::SameLine();

		// Rotate button (E)
		bool isRotateSelected = (m_GuizmoType == ImGuizmo::OPERATION::ROTATE);
		if (isRotateSelected)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.5f, 0.6f, 0.9f));
		if (ImGui::Button("E", ImVec2(buttonSize, buttonSize)))
			m_GuizmoType = ImGuizmo::OPERATION::ROTATE;
		if (isRotateSelected)
			ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Rotate Mode (E)");

		ImGui::SameLine();

		// Scale button (R)
		bool isScaleSelected = (m_GuizmoType == ImGuizmo::OPERATION::SCALE);
		if (isScaleSelected)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.5f, 0.6f, 0.9f));
		if (ImGui::Button("R", ImVec2(buttonSize, buttonSize)))
			m_GuizmoType = ImGuizmo::OPERATION::SCALE;
		if (isScaleSelected)
			ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Scale Mode (R)");

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
	}

}