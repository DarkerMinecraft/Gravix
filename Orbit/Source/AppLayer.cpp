#include "AppLayer.h"

#include "Utils/PlatformUtils.h"
#include "Serialization/Scene/SceneSerializer.h"
#include "Events/KeyEvents.h"
#include "Events/WindowEvents.h"

#include "Asset/Importers/TextureImporter.h"
#include "Debug/Instrumentor.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

namespace Gravix
{

	AppLayer::AppLayer()
	{
		GX_PROFILE_FUNCTION();
		// Setup manager callbacks
		m_ProjectManager.SetOnProjectLoadedCallback([this]() { InitializeProject(); });
		m_ProjectManager.SetOnProjectCreatedCallback([this]() { InitializeProject(); });

		m_SceneManager.SetOnSceneChangedCallback([this]() {
			m_SceneHierarchyPanel.SetContext(m_SceneManager.GetActiveScene());
			m_SceneHierarchyPanel.SetNoneSelected();
			UpdateWindowTitle();
		});

		m_SceneManager.SetOnSceneDirtyCallback([this]() {
			UpdateWindowTitle();
		});

		m_SceneManager.SetOnScenePlayCallback([this]() {
			auto& viewportSize = m_ViewportPanel.GetViewportSize();
			m_SceneManager.GetActiveScene()->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		});

		// Check if a project has been loaded
		if (Project::HasActiveProject())
		{
			InitializeProject();
		}
		else
		{
			// Show startup dialog to prompt user to open/create a project
			m_ProjectManager.SetShowStartupDialog(true);
			m_ProjectManager.CreateNewProject();
		}

		FramebufferSpecification fbSpec{};
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RedFloat, FramebufferTextureFormat::Depth };
		fbSpec.Multisampled = true;

		m_MSAAFramebuffer = Framebuffer::Create(fbSpec);
		m_MSAAFramebuffer->SetClearColor(0, glm::vec4{ 0.1f, 0.1f, 0.1f, 1.0f });
		m_MSAAFramebuffer->SetClearColor(1, glm::vec4{ -1.0f, 0.0f, 0.0f, 0.0f });

		fbSpec.Multisampled = false;
		m_FinalFramebuffer = Framebuffer::Create(fbSpec);

		m_SceneHierarchyPanel.SetContext(m_SceneManager.GetActiveScene());
		m_SceneHierarchyPanel.SetAppLayer(this);
		m_InspectorPanel.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_InspectorPanel.SetAppLayer(this);
		m_ViewportPanel.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_ViewportPanel.SetFramebuffer(m_FinalFramebuffer, 0);

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);
		m_ViewportPanel.SetEditorCamera(&m_EditorCamera);
		m_ViewportPanel.SetAppLayer(this);

		m_ContentBrowserPanel.emplace();
		m_ContentBrowserPanel->SetAppLayer(this);

		Renderer2D::Init(m_MSAAFramebuffer);

		m_IconPlay = TextureImporter::LoadTexture2D("EditorAssets/Icons/PlayButton.png");
		m_IconStop = TextureImporter::LoadTexture2D("EditorAssets/Icons/StopButton.png");
	}

	void AppLayer::InitializeProject()
	{
		GX_PROFILE_FUNCTION();

		// Load the start scene
		Ref<Scene> scene = m_SceneManager.LoadStartScene(m_ViewportPanel.GetViewportSize());

		// Update panels
		m_SceneHierarchyPanel.SetContext(scene);
		m_SceneHierarchyPanel.SetNoneSelected();

		m_ProjectInitialized = true;
		m_ProjectManager.SetShowStartupDialog(false);

		// Update window title with scene name
		UpdateWindowTitle();
	}

	AppLayer::~AppLayer()
	{
		GX_PROFILE_FUNCTION();

		if (Project::HasActiveProject())
		{
			Project::GetActive()->GetEditorAssetManager()->ClearLoadedAssets();
		}
		Renderer2D::Destroy();
	}

	void AppLayer::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(AppLayer::OnKeyPressed));
		dispatcher.Dispatch<WindowFileDropEvent>(BIND_EVENT_FN(AppLayer::OnFileDrop));

		// Only forward events to panels if project is initialized
		if (m_ProjectInitialized)
		{
			if (m_SceneManager.GetSceneState() == SceneState::Edit)
				m_EditorCamera.OnEvent(e);
			m_ViewportPanel.OnEvent(e);
		}
	}

	void AppLayer::OnUpdate(float deltaTime)
	{
		GX_PROFILE_FUNCTION();

		// Only update if project is initialized
		if (!m_ProjectInitialized)
			return;

		// Check if pending scene has finished loading
		{
			GX_PROFILE_SCOPE("CheckPendingScene");
			AssetHandle pendingScene = m_SceneManager.GetPendingSceneHandle();
			if (pendingScene != 0 && AssetManager::IsAssetLoaded(pendingScene))
			{
				GX_CORE_INFO("Async scene load completed, switching to scene {0}", static_cast<uint64_t>(pendingScene));
				m_SceneManager.ClearPendingScene();
				m_SceneManager.OpenScene(pendingScene, false); // AssetManager already deserialized the scene
			}
		}

		{
			GX_PROFILE_SCOPE("ViewportResize");
			if (m_ViewportPanel.IsViewportValid())
			{
				auto& viewportSize = m_ViewportPanel.GetViewportSize();

				m_MSAAFramebuffer->Resize(viewportSize.x, viewportSize.y);
				m_SceneManager.GetActiveScene()->OnViewportResize(viewportSize.x, viewportSize.y);

				m_ViewportPanel.ResizeFramebuffer();
				m_EditorCamera.SetViewportSize(viewportSize.x, viewportSize.y);

				m_ViewportPanel.UpdateViewport();
			}
		}

		{
			GX_PROFILE_SCOPE("SceneUpdate");
			if (m_SceneManager.GetSceneState() == SceneState::Edit)
			{
				m_SceneManager.GetActiveScene()->OnEditorUpdate(deltaTime);

				if (m_ViewportPanel.IsViewportHovered())
					m_EditorCamera.OnUpdate(deltaTime);
			}
			else
			{
				m_SceneManager.GetActiveScene()->OnRuntimeUpdate(deltaTime);
			}
		}

		{
			GX_PROFILE_SCOPE("EntitySelection");
			// Entity selection on click (works in both Edit and Play modes)
			if (m_ViewportPanel.IsViewportHovered() && Input::IsMouseDown(Mouse::LeftButton))
			{
				// Don't select entities when using ImGuizmo
				if (!ImGuizmo::IsUsing() && !ImGuizmo::IsOver())
				{
					Entity hoveredEntity = m_ViewportPanel.GetHoveredEntity();

					if (hoveredEntity)
					{
						m_SceneHierarchyPanel.SetSelectedEntity(hoveredEntity);
					}
				}
			}
		}
	}

	void AppLayer::OnRender()
	{
		GX_PROFILE_FUNCTION();

		// Only render if project is initialized
		if (!m_ProjectInitialized)
			return;

		{
			GX_PROFILE_SCOPE("SceneRender");
			Command cmd(m_MSAAFramebuffer, 0, false);

			cmd.BeginRendering();
			if (m_SceneManager.GetSceneState() == SceneState::Edit)
				m_SceneManager.GetActiveScene()->OnEditorRender(cmd, m_EditorCamera);
			else
				m_SceneManager.GetActiveScene()->OnRuntimeRender(cmd);
			cmd.EndRendering();

			cmd.ResolveFramebuffer(m_FinalFramebuffer, true);
		}
	}

	void AppLayer::OnImGuiRender()
	{
		GX_PROFILE_FUNCTION();

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
					if (m_ProjectManager.CreateNewProject())
					{
						// Refresh content browser panel
						m_ContentBrowserPanel.emplace();
						m_ContentBrowserPanel->SetAppLayer(this);
					}
				}

				if (ImGui::MenuItem("Open... ", "Ctrl+O"))
				{
					if (m_ProjectManager.OpenProject())
					{
						// Refresh content browser panel
						m_ContentBrowserPanel.emplace();
						m_ContentBrowserPanel->SetAppLayer(this);
					}
				}

				if (m_ProjectInitialized && ImGui::MenuItem("Save", "Ctrl+S"))
				{
					m_SceneManager.SaveActiveScene();
				}

				if (m_ProjectInitialized && ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
				{
					//SaveSceneAs();
				}

				ImGui::Separator();

				if (m_ProjectInitialized && ImGui::MenuItem("Preferences..."))
				{
					m_ProjectSettingsPanel.Open();
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		// Show startup dialog if no project is loaded
		if (m_ProjectManager.ShouldShowStartupDialog())
		{
			ShowStartupDialog();
		}

		// Only render panels if project is initialized
		if (m_ProjectInitialized)
		{
			m_ViewportPanel.OnImGuiRender();
			m_SceneHierarchyPanel.OnImGuiRender();
			m_InspectorPanel.OnImGuiRender();
			if (m_ContentBrowserPanel) m_ContentBrowserPanel->OnImGuiRender();
			m_ProjectSettingsPanel.OnImGuiRender();
			UIToolbar();
		}
		ImGui::End();
	}

	void AppLayer::UIToolbar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		auto& colors = ImGui::GetStyle().Colors;
		auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
		auto& buttonActive = colors[ImGuiCol_ButtonActive];
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.x, 0.5f));

		ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		float windowHeight = ImGui::GetWindowHeight();
		float buttonSize = windowHeight * 0.8f; // 80% of window height for some breathing room

		Ref<Texture2D> icon = (m_SceneManager.GetSceneState() == SceneState::Edit) ? m_IconPlay : m_IconStop;
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (buttonSize * 0.5f));
		if (ImGui::ImageButton("SceneState", (ImTextureID)icon->GetImGuiAttachment(), ImVec2{ buttonSize, buttonSize }))
		{
			if (m_SceneManager.GetSceneState() == SceneState::Edit)
				m_SceneManager.Play();
			else if (m_SceneManager.GetSceneState() == SceneState::Play)
				m_SceneManager.Stop();
		}

		ImGui::PopStyleVar(4);
		ImGui::PopStyleColor(3);
		ImGui::End();
	}




	bool AppLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// Don't process shortcuts if ImGui wants keyboard input
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureKeyboard && (io.WantTextInput || ImGui::IsAnyItemActive()))
			return false;

		bool ctrlDown = Input::IsKeyDown(Key::LeftControl) || Input::IsKeyDown(Key::RightControl);
		bool shiftDown = Input::IsKeyDown(Key::LeftShift) || Input::IsKeyDown(Key::RightShift);

		switch (e.GetKeyCode())
		{
		case Key::N:
			if (ctrlDown)
			{
				if (m_ProjectManager.CreateNewProject())
				{
					// Refresh content browser panel
					m_ContentBrowserPanel.emplace();
					m_ContentBrowserPanel->SetAppLayer(this);
				}
				return true;
			}
			break;

		case Key::O:
			if (ctrlDown)
			{
				if (m_ProjectManager.OpenProject())
				{
					// Refresh content browser panel
					m_ContentBrowserPanel.emplace();
					m_ContentBrowserPanel->SetAppLayer(this);
				}
				return true;
			}
			break;

		case Key::S:
			if (ctrlDown)
			{
				if (shiftDown)
				{
					m_ProjectManager.SaveActiveProjectAs();
				}
				else
				{
					m_SceneManager.SaveActiveScene();
				}
				return true;
			}
			break;
		}

		return false;
	}

	bool AppLayer::OnFileDrop(WindowFileDropEvent& e)
	{
		if (m_ContentBrowserPanel) m_ContentBrowserPanel->OnFileDrop(e.GetPaths());
		return true;
	}


	void AppLayer::ShowStartupDialog()
	{
		// Center the modal dialog
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);

		// Open the modal on first frame
		if (!ImGui::IsPopupOpen("Welcome to Orbit"))
		{
			ImGui::OpenPopup("Welcome to Orbit");
		}

		// Use a modal window
		if (ImGui::BeginPopupModal("Welcome to Orbit", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::Spacing();
			ImGui::Spacing();

			// Title
			ImGui::PushFont(io.Fonts->Fonts[1]); // Bold font
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Welcome to Orbit Editor").x) * 0.5f);
			ImGui::Text("Welcome to Orbit Editor");
			ImGui::PopFont();

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Spacing();

			// Description
			ImGui::TextWrapped("To get started, please create a new project or open an existing one.");

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			// Center the buttons
			float buttonWidth = 150.0f;
			float spacing = 20.0f;
			float totalWidth = buttonWidth * 2 + spacing;
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

			// New Project button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.267f, 0.529f, 0.808f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.353f, 0.627f, 0.902f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.208f, 0.471f, 0.749f, 1.0f));
			ImGui::AlignTextToFramePadding();

			if (ImGui::Button("New Project", ImVec2(buttonWidth, 40)))
			{
				if (m_ProjectManager.CreateNewProject())
				{
					// Refresh content browser panel
					m_ContentBrowserPanel.emplace();
					m_ContentBrowserPanel->SetAppLayer(this);
				}
				ImGui::CloseCurrentPopup();
			}

			ImGui::PopStyleColor(3);

			ImGui::SameLine(0, spacing);
			ImGui::AlignTextToFramePadding();

			// Open Project button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.267f, 0.267f, 0.267f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.349f, 0.349f, 0.349f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.443f, 0.443f, 0.443f, 1.0f));

			if (ImGui::Button("Open Project", ImVec2(buttonWidth, 40)))
			{
				if (m_ProjectManager.OpenProject())
				{
					// Refresh content browser panel
					m_ContentBrowserPanel.emplace();
					m_ContentBrowserPanel->SetAppLayer(this);
				}
				ImGui::CloseCurrentPopup();
			}

			ImGui::PopStyleColor(3);

			ImGui::EndPopup();
		}
	}


	void AppLayer::UpdateWindowTitle()
	{
		if (!Project::HasActiveProject())
		{
			Application::Get().GetWindow().SetTitle("Orbit");
			return;
		}

		std::string sceneName = "Untitled";

		AssetHandle activeSceneHandle = m_SceneManager.GetActiveSceneHandle();
		if (activeSceneHandle != 0 && AssetManager::IsValidAssetHandle(activeSceneHandle))
		{
			const auto& metadata = Project::GetActive()->GetEditorAssetManager()->GetAssetMetadata(activeSceneHandle);
			sceneName = metadata.FilePath.stem().string();
		}

		std::string title = "Orbit - " + sceneName;
		if (m_SceneManager.IsSceneDirty())
			title += "*";

		Application::Get().GetWindow().SetTitle(title);
	}

	void AppLayer::MarkSceneDirty()
	{
		m_SceneManager.MarkSceneDirty();
	}

	void AppLayer::OpenScene(AssetHandle handle, bool deserialize)
	{
		if (m_SceneManager.OpenScene(handle, deserialize))
		{
			// Update viewport size for the new scene
			auto& viewportSize = m_ViewportPanel.GetViewportSize();
			m_SceneManager.GetActiveScene()->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		}
	}

}