#include "AppLayer.h"

#include "Command.h"

#include <imgui.h>

namespace Orbit 
{

	AppLayer::AppLayer()
	{
		Gravix::FramebufferSpecification fbSpec{};
		fbSpec.Attachments = { Gravix::FramebufferTextureFormat::RGBA8 };

		m_MainFramebuffer = Gravix::Framebuffer::Create(fbSpec);
		m_MainFramebuffer->SetClearColor(0, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	AppLayer::~AppLayer()
	{

	}

	void AppLayer::OnEvent(Gravix::Event& event)
	{

	}

	void AppLayer::OnUpdate(float deltaTime)
	{
		m_MainFramebuffer->Resize(m_ViewportSize.x, m_ViewportSize.y);
	}

	void AppLayer::OnRender()
	{
		Gravix::Command cmd(m_MainFramebuffer, 0, false);

		cmd.BeginRendering();
		cmd.EndRendering();
	}

	void AppLayer::OnImGuiRender()
	{
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static bool* p_open = NULL;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Orbit Editor", p_open, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{

			ImGui::EndMenuBar();
		}

		DrawViewportUI();

		ImGui::End();
	}

	void AppLayer::DrawViewportUI()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");
		ImVec2 avail = ImGui::GetContentRegionAvail();
		m_ViewportSize = { avail.x, avail.y };

		ImGui::Image(m_MainFramebuffer->GetColorAttachmentID(0), avail);
		ImGui::End();
		ImGui::PopStyleVar();
	}

}