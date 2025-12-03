#include "AppLayer.h"

#include "Events/KeyEvents.h"
#include "Events/WindowEvents.h"

#include "Scripting/Core/ScriptEngine.h"
#include "Scripting/Editor/EditorScriptEngine.h"
#include "Scripting/Fields/ScriptFieldRegistry.h"

#include "Asset/EditorAssetManager.h"

#include "Utils/ShaderCompilerSystem.h"
#include "Debug/Instrumentor.h"

#include <imgui.h>
#include <ImGuizmo.h>

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
			m_ViewportPanel.SetImGuizmoNone();
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
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RedInt, FramebufferTextureFormat::Depth };
		fbSpec.Multisampled = true;

		m_MSAAFramebuffer = Framebuffer::Create(fbSpec);
		m_MSAAFramebuffer->SetClearColor(0, glm::vec4{ 0.1f, 0.1f, 0.1f, 1.0f });
		m_MSAAFramebuffer->SetClearColor(1, glm::ivec4{ -1, 0, 0, 0 });

		fbSpec.Multisampled = false;
		m_FinalFramebuffer = Framebuffer::Create(fbSpec);
		m_FinalFramebuffer->SetClearColor(0, glm::vec4{ 0.1f, 0.1f, 0.1f, 1.0f });
		m_FinalFramebuffer->SetClearColor(1, glm::ivec4{ -1, 0, 0, 0 });

		m_SceneHierarchyPanel.SetContext(m_SceneManager.GetActiveScene());
		m_SceneHierarchyPanel.SetAppLayer(this);
		m_InspectorPanel.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_InspectorPanel.SetAppLayer(this);
		m_ViewportPanel.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_ViewportPanel.SetFramebuffer(m_FinalFramebuffer, 0);

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);
		m_ViewportPanel.SetEditorCamera(&m_EditorCamera);
		m_ViewportPanel.SetAppLayer(this);
		m_ViewportPanel.SetSceneManager(&m_SceneManager);
		m_ViewportPanel.LoadIcons();

		m_ContentBrowserPanel.emplace();
		m_ContentBrowserPanel->SetAppLayer(this);

		// Setup UI components
		m_MenuBar.SetProjectManager(&m_ProjectManager);
		m_MenuBar.SetSceneManager(&m_SceneManager);
		m_MenuBar.SetProjectSettingsPanel(&m_ProjectSettingsPanel);
		m_MenuBar.SetOnProjectCreatedCallback([this]() { RefreshContentBrowser(); });
		m_MenuBar.SetOnProjectOpenedCallback([this]() { RefreshContentBrowser(); });

		m_Toolbar.SetSceneManager(&m_SceneManager);

		m_ShortcutHandler.SetProjectManager(&m_ProjectManager);
		m_ShortcutHandler.SetSceneManager(&m_SceneManager);
		m_ShortcutHandler.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_ShortcutHandler.SetOnProjectCreatedCallback([this]() { RefreshContentBrowser(); });
		m_ShortcutHandler.SetOnProjectOpenedCallback([this]() { RefreshContentBrowser(); });

		Renderer2D::Init(m_MSAAFramebuffer);
	}

	void AppLayer::InitializeProject()
	{
		GX_PROFILE_FUNCTION();

		// Shutdown previous project's script engine if switching projects
		if (m_ProjectInitialized)
		{
			ScriptEngine::Shutdown();
			// Clear script field registry when switching projects
			ScriptEngine::GetFieldRegistry().Clear();
		}

		// Initialize global ShaderCompiler system (editor only)
		ShaderCompilerSystem::Initialize();
		ScriptEngine::Initialize();

		// Load centralized script field registry
		std::filesystem::path registryPath = Project::GetActive()->GetConfig().LibraryDirectory / "ScriptsRegistry.orbreg";
		if (std::filesystem::exists(registryPath))
		{
			ScriptEngine::GetFieldRegistry().Deserialize(registryPath);
			GX_CORE_INFO("Loaded script field registry from: {0}", registryPath.string());
		}

		// Start file watchers for hot reload
		{
			try
			{
				// Start watching scripts for hot reload (scripts are in Assets directory)
				std::filesystem::path assetPath = Project::GetActive()->GetConfig().AssetDirectory;
				if (std::filesystem::exists(assetPath))
				{
					EditorScriptEngine::StartWatchingScripts(assetPath);
					GX_CORE_INFO("Watching for C# script changes in: {0}", assetPath.string());
				}
				else
				{
					GX_CORE_WARN("Asset directory does not exist: {0}", assetPath.string());
				}
			}
			catch (const std::exception& e)
			{
				GX_CORE_ERROR("Failed to start script file watcher: {0}", e.what());
			}

			try
			{
				// Start watching assets for hot reload
				std::filesystem::path assetPath = Project::GetActive()->GetConfig().AssetDirectory;
				if (std::filesystem::exists(assetPath))
				{
					auto assetManager = Project::GetActive()->GetEditorAssetManager();
					if (assetManager)
					{
						assetManager->StartWatchingAssets(assetPath);
					}
				}
				else
				{
					GX_CORE_WARN("Asset path does not exist: {0}", assetPath.string());
				}
			}
			catch (const std::exception& e)
			{
				GX_CORE_ERROR("Failed to start asset file watcher: {0}", e.what());
			}
		}

		// Load the start scene
		Ref<Scene> scene = m_SceneManager.LoadStartScene(m_ViewportPanel.GetViewportSize());

		// Update panels
		m_SceneHierarchyPanel.SetContext(scene);
		m_SceneHierarchyPanel.SetNoneSelected();

		m_ProjectInitialized = true;
		m_ProjectManager.SetShowStartupDialog(false);

		// Update window title with scene name
		UpdateWindowTitle();

		// Update MenuBar with ContentBrowserPanel pointer
		if (m_ContentBrowserPanel)
			m_MenuBar.SetContentBrowserPanel(&(*m_ContentBrowserPanel));
	}

	void AppLayer::RefreshContentBrowser()
	{
		m_ContentBrowserPanel.emplace();
		m_ContentBrowserPanel->SetAppLayer(this);
		if (m_ContentBrowserPanel)
			m_MenuBar.SetContentBrowserPanel(&(*m_ContentBrowserPanel));
	}

	AppLayer::~AppLayer()
	{
		GX_PROFILE_FUNCTION();

		if (m_ProjectInitialized)
		{
			ScriptEngine::Shutdown();
		}

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
			if (m_SceneManager.GetSceneState() == SceneState::Edit && m_ViewportPanel.IsViewportHovered() && m_ViewportPanel.IsViewportFocused())
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

		// Process file watcher changes (hot reload)
		{
			GX_PROFILE_SCOPE("FileWatcherUpdate");

			try
			{
				// Check for script changes and hot reload
				EditorScriptEngine::CheckForScriptReload();
			}
			catch (const std::exception& e)
			{
				GX_CORE_ERROR("Error during script reload check: {0}", e.what());
			}

			try
			{
				// Process asset changes
				auto assetManager = Project::GetActive()->GetEditorAssetManager();
				if (assetManager)
				{
					assetManager->ProcessAssetChanges();
				}
			}
			catch (const std::exception& e)
			{
				GX_CORE_ERROR("Error during asset change processing: {0}", e.what());
			}
		}

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

		// Only resize if viewport size actually changed
		if (m_ViewportPanel.IsViewportValid())
		{
			auto& viewportSize = m_ViewportPanel.GetViewportSize();

			// Check if size changed
			if (viewportSize.x != m_LastViewportSize.x || viewportSize.y != m_LastViewportSize.y)
			{
				GX_PROFILE_SCOPE("ViewportResize");
				m_LastViewportSize = viewportSize;

				m_MSAAFramebuffer->Resize(viewportSize.x, viewportSize.y);
				if(m_SceneManager.IsValidScene())
					m_SceneManager.GetActiveScene()->OnViewportResize(viewportSize.x, viewportSize.y);

				m_ViewportPanel.ResizeFramebuffer();
				m_EditorCamera.SetViewportSize(viewportSize.x, viewportSize.y);
			}

			// Update viewport mouse picking every frame (cheap operation)
			m_ViewportPanel.UpdateViewport();
		}

		{
			GX_PROFILE_SCOPE("SceneUpdate");
			if (m_SceneManager.IsValidScene()) 
			{
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
		}

		{
			GX_PROFILE_SCOPE("EntitySelection");
			// Entity selection on click (works in both Edit and Play modes)
			if (m_ViewportPanel.IsViewportHovered() && m_ViewportPanel.IsViewportFocused() && Input::IsMouseDown(Mouse::LeftButton))
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
			if (m_SceneManager.IsValidScene())
			{
				Command cmd(m_MSAAFramebuffer, 0, false);

				cmd.BeginRendering();
				if (m_SceneManager.GetSceneState() == SceneState::Edit) m_SceneManager.GetActiveScene()->OnEditorRender(cmd, m_EditorCamera);
				else m_SceneManager.GetActiveScene()->OnRuntimeRender(cmd);
				OnOverlayRender(cmd);
				cmd.EndRendering();

				cmd.ResolveFramebuffer(m_FinalFramebuffer, true);
			}
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

		if (m_ProjectInitialized)
			m_MenuBar.OnImGuiRender();

		// Only render panels if project is initialized
		if (m_ProjectInitialized)
		{
			m_ViewportPanel.OnImGuiRender();
			m_SceneHierarchyPanel.OnImGuiRender();
			m_InspectorPanel.OnImGuiRender();
			if (m_ContentBrowserPanel) m_ContentBrowserPanel->OnImGuiRender();
			m_ProjectSettingsPanel.OnImGuiRender();
			m_ConsolePanel.OnImGuiRender();
			// Toolbar is now integrated into ViewportPanel
			UISettings();
		}
		ImGui::End();
	}

	void AppLayer::UISettings()
	{
		ImGui::Begin("Settings");
		ImGui::Checkbox("Show Physics Colliders", &m_ShowPhysicsColliders);
		ImGui::End();
	}

	void AppLayer::OnOverlayRender(Command& cmd)
	{

		if (m_SceneManager.GetSceneState() == SceneState::Edit)
		{
			Renderer2D::BeginScene(cmd, m_EditorCamera);
		}
		else
		{
			glm::mat4 transform;
			auto cam = m_SceneManager.GetPrimaryCamera(&transform);
			Renderer2D::BeginScene(cmd, cam, transform);
		}

		if (m_ShowPhysicsColliders) {
			{
				auto view = m_SceneManager.GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
				view.each([&](auto entityID, TransformComponent& tc, CircleCollider2DComponent& collider)
					{
						glm::vec3 translation = tc.Position + glm::vec3(collider.Offset.x, collider.Offset.y, 0.001f);
						glm::vec3 scale = tc.Scale * glm::vec3(collider.Size, 1.0f);
						glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
							* glm::scale(glm::mat4(1.0f), scale);

						Renderer2D::DrawCircleOutline(transform, { 0.0, 1.0f, 0.0f, 1.0f });
					});
			}

			{
				auto view = m_SceneManager.GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
				view.each([&](auto entityID, TransformComponent& tc, BoxCollider2DComponent& collider)
					{
						glm::vec3 translation = tc.Position + glm::vec3(collider.Offset.x, collider.Offset.y, 0.001f);
						glm::vec3 scale = tc.Scale * glm::vec3(collider.Size.x * 2.0f, collider.Size.y * 2.0f, 1.0f);
						glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
							* glm::rotate(glm::mat4(1.0f), glm::radians(tc.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f))
							* glm::scale(glm::mat4(1.0f), scale);

						Renderer2D::DrawQuadOutline(transform, { 0.0, 1.0f, 0.0f, 1.0f });
					});
			}
		}
		Renderer2D::EndScene(cmd);
	}

	bool AppLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		return m_ShortcutHandler.HandleKeyPress(e);
	}

	bool AppLayer::OnFileDrop(WindowFileDropEvent& e)
	{
		if (m_ContentBrowserPanel) m_ContentBrowserPanel->OnFileDrop(e.GetPaths());
		return true;
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
		if (m_SceneManager.GetSceneState() != SceneState::Edit)
			m_SceneManager.Stop();

		if (m_SceneManager.OpenScene(handle, deserialize))
		{
			// Update viewport size for the new scene
			auto& viewportSize = m_ViewportPanel.GetViewportSize();
			m_SceneManager.GetActiveScene()->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		}
	}

}