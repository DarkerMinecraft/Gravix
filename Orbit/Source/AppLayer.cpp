#include "AppLayer.h"

#include "Utils/PlatformUtils.h"
#include "Serialization/Scene/SceneSerializer.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Gravix
{

	AppLayer::AppLayer()
	{

		FramebufferSpecification fbSpec{};
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RedFloat };
		fbSpec.Multisampled = true;

		m_MSAAFramebuffer = Framebuffer::Create(fbSpec);
		m_MSAAFramebuffer->SetClearColor(0, glm::vec4{ 0.1f, 0.1f, 0.1f, 1.0f });
		m_MSAAFramebuffer->SetClearColor(1, glm::vec4{ -1.0f, 0.0f, 0.0f, 0.0f });

		fbSpec.Multisampled = false;
		m_FinalFramebuffer = Framebuffer::Create(fbSpec);

		Renderer2D::Init(m_MSAAFramebuffer);

		OpenScene(Project::GetActiveConfig().StartScene);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_InspectorPanel.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_ViewportPanel.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_ViewportPanel.SetFramebuffer(m_FinalFramebuffer, 0);

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);
		m_ViewportPanel.SetEditorCamera(&m_EditorCamera);
		m_ViewportPanel.SetAppLayer(this);

		m_ContentBrowserPanel = ContentBroswerPanel();
	}

	AppLayer::~AppLayer()
	{
		Renderer2D::Destroy();
	}

	void AppLayer::OnEvent(Event& e)
	{
		m_EditorCamera.OnEvent(e);
	}

	void AppLayer::OnUpdate(float deltaTime)
	{
		if (m_ViewportPanel.IsViewportValid())
		{
			auto& viewportSize = m_ViewportPanel.GetViewportSize();

			m_MSAAFramebuffer->Resize(viewportSize.x, viewportSize.y);
			m_ActiveScene->OnViewportResize(viewportSize.x, viewportSize.y);

			m_ViewportPanel.ResizeFramebuffer();
			m_EditorCamera.SetViewportSize(viewportSize.x, viewportSize.y);

			m_ViewportPanel.UpdateViewport();
		}

		if (m_ViewportPanel.IsViewportHovered())
			m_EditorCamera.OnUpdate(deltaTime);

		m_ActiveScene->OnEditorUpdate(deltaTime);

		OnShortcuts();
		m_ViewportPanel.GuizmoShortcuts();

		if (Input::IsMouseDown(Mouse::LeftButton))
		{
			if (m_ViewportPanel.IsViewportHovered())
			{
				Entity hoveredEntity = m_ViewportPanel.GetHoveredEntity();

				if (hoveredEntity)
				{
					m_SceneHierarchyPanel.SetSelectedEntity(hoveredEntity);
				}
			}
		}
	}

	void AppLayer::OnRender()
	{
		{
			Command cmd(m_MSAAFramebuffer, 0, false);

			cmd.BeginRendering();
			m_ActiveScene->OnEditorRender(cmd, m_EditorCamera);
			cmd.EndRendering();

			cmd.ResolveFramebuffer(m_FinalFramebuffer, true);
		}
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
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					NewScene();
				}

				if (ImGui::MenuItem("Open... ", "Ctrl+O"))
				{
					OpenScene();
				}

				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					SaveScene();
				}

				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
				{
					SaveSceneAs();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		m_ViewportPanel.OnImGuiRender();
		m_SceneHierarchyPanel.OnImGuiRender();
		m_InspectorPanel.OnImGuiRender();
		m_ContentBrowserPanel.OnImGuiRender();
		ImGui::End();
	}

	void AppLayer::OnShortcuts()
	{
		bool ctrlDown = Input::IsKeyDown(Key::LeftControl) || Input::IsKeyDown(Key::RightControl);
		bool shiftDown = Input::IsKeyDown(Key::LeftShift) || Input::IsKeyDown(Key::RightShift);

		if (ctrlDown && Input::IsKeyPressed(Key::S))
		{
			if (shiftDown)
				SaveSceneAs();
			else
				SaveScene();
		}

		if (ctrlDown && Input::IsKeyPressed(Key::O))
		{
			OpenScene();
		}

		if (ctrlDown && Input::IsKeyPressed(Key::N))
		{
			NewScene();
		}
	}

	void AppLayer::SaveProject()
	{
		if(m_ActiveProjectPath.empty())
		{
			std::filesystem::path filePath = FileDialogs::SaveFile("Orbit Project (*.orbproj)\0*.orbproj\0");
			if (filePath.empty())
				return;

			m_ActiveProjectPath = filePath;
		}
		Project::SaveActive(m_ActiveProjectPath);
	}

	void AppLayer::SaveProjectAs()
	{
		std::filesystem::path filePath = FileDialogs::SaveFile("Orbit Project (*.orbproj)\0*.orbproj\0");
		if (filePath.empty())
			return;

		m_ActiveProjectPath = filePath;
		Project::SaveActive(m_ActiveProjectPath);
	}

	void AppLayer::OpenProject()
	{
		m_ActiveProjectPath = FileDialogs::OpenFile("Orbit Project (*.orbproj)\0*.orbproj\0");
		if (m_ActiveProjectPath.empty())
			return;

		Project::Load(m_ActiveProjectPath);
	}

	void AppLayer::NewProject()
	{
		Project::New();
	}

	void AppLayer::SaveScene()
	{
		if (m_ActiveScenePath.empty())
		{
			std::filesystem::path filePath = FileDialogs::SaveFile("Orbit Scene (*.orbscene)\0*.orbscene\0");
			if (filePath.empty())
				return;

			m_ActiveScenePath = filePath;
		}

		SceneSerializer serializer(m_ActiveScene);
		serializer.Serialize(m_ActiveScenePath);
	}

	void AppLayer::SaveSceneAs()
	{
		std::filesystem::path filePath = FileDialogs::SaveFile("Orbit Scene (*.orbscene)\0*.orbscene\0");
		if (filePath.empty())
			return;
		m_ActiveScenePath = filePath;

		SceneSerializer serializer(m_ActiveScene);
		serializer.Serialize(m_ActiveScenePath);
	}

	void AppLayer::OpenScene()
	{
		std::filesystem::path filePath = FileDialogs::OpenFile("Orbit Scene (*.orbscene)\0*.orbscene\0");
		if (filePath.empty())
			return;
		m_ActiveScenePath = filePath;

		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportPanel.GetViewportSize().x, (uint32_t)m_ViewportPanel.GetViewportSize().y);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_SceneHierarchyPanel.SetNoneSelected();

		SceneSerializer serializer(m_ActiveScene);
		serializer.Deserialize(filePath);
	}

	void AppLayer::OpenScene(const std::filesystem::path& path)
	{
		m_ActiveScenePath = Project::GetAssetDirectory() / path;

		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportPanel.GetViewportSize().x, (uint32_t)m_ViewportPanel.GetViewportSize().y);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_SceneHierarchyPanel.SetNoneSelected();

		SceneSerializer serializer(m_ActiveScene);
		serializer.Deserialize(m_ActiveScenePath);
	}

	void AppLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportPanel.GetViewportSize().x, (uint32_t)m_ViewportPanel.GetViewportSize().y);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_SceneHierarchyPanel.SetNoneSelected();

		m_ActiveScenePath = std::filesystem::path();
	}

}