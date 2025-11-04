#include "ViewportPanel.h"

#include <imgui.h>

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

		ImVec2 avail = ImGui::GetContentRegionAvail();
		m_ViewportSize = { avail.x, avail.y };

		ImVec2 uv0 = ImVec2(0, 0);
		ImVec2 uv1 = ImVec2(1, 1); 

		ImGui::Image(m_Framebuffer->GetColorAttachmentID(m_RenderIndex), avail, uv0, uv1);

		ImGui::End();
		ImGui::PopStyleVar();
	}
}