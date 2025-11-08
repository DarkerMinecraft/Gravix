#include "AppLayer.h"

#include "Utils/PlatformUtils.h"
#include "Serialization/Scene/SceneSerializer.h"
#include "Events/KeyEvents.h"
#include "Events/WindowEvents.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Gravix
{

	AppLayer::AppLayer()
	{

		FramebufferSpecification fbSpec{};
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RedFloat, FramebufferTextureFormat::Depth };
		fbSpec.Multisampled = true;

		m_MSAAFramebuffer = Framebuffer::Create(fbSpec);
		m_MSAAFramebuffer->SetClearColor(0, glm::vec4{ 0.1f, 0.1f, 0.1f, 1.0f });
		m_MSAAFramebuffer->SetClearColor(1, glm::vec4{ -1.0f, 0.0f, 0.0f, 0.0f });

		fbSpec.Multisampled = false;
		m_FinalFramebuffer = Framebuffer::Create(fbSpec);

		Renderer2D::Init(m_MSAAFramebuffer);

		// Always create a default empty scene to prevent nullptr issues
		m_ActiveScene = CreateRef<Scene>();

		// Try to open the start scene if it's valid
		AssetHandle startScene = Project::GetActive()->GetConfig().StartScene;
		if (startScene != 0)
		{
			OpenScene(startScene);
		}

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_InspectorPanel.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_ViewportPanel.SetSceneHierarchyPanel(&m_SceneHierarchyPanel);
		m_ViewportPanel.SetFramebuffer(m_FinalFramebuffer, 0);

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);
		m_ViewportPanel.SetEditorCamera(&m_EditorCamera);
		m_ViewportPanel.SetAppLayer(this);

		m_ContentBrowserPanel = ContentBrowserPanel();
	}

	AppLayer::~AppLayer()
	{
		Project::GetActive()->GetEditorAssetManager()->ClearLoadedAssets();
		Renderer2D::Destroy();
	}

	void AppLayer::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(AppLayer::OnKeyPressed));
		dispatcher.Dispatch<WindowFileDropEvent>(BIND_EVENT_FN(AppLayer::OnFileDrop));

		m_EditorCamera.OnEvent(e);
		m_ViewportPanel.OnEvent(e);
	}

	void AppLayer::OnUpdate(float deltaTime)
	{
		// Check if pending scene has finished loading
		if (m_PendingSceneHandle != 0 && AssetManager::IsAssetLoaded(m_PendingSceneHandle))
		{
			GX_CORE_INFO("Async scene load completed, switching to scene {0}", static_cast<uint64_t>(m_PendingSceneHandle));
			AssetHandle sceneToLoad = m_PendingSceneHandle;
			m_PendingSceneHandle = 0; // Clear pending before calling OpenScene
			OpenScene(sceneToLoad, true); // Load with deserialization
		}

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
					NewProject();
				}

				if (ImGui::MenuItem("Open... ", "Ctrl+O"))
				{
					OpenProject();
				}

				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					SaveScene();
				}

				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
				{
					//SaveSceneAs();
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
				NewProject();
				return true;
			}
			break;

		case Key::O:
			if (ctrlDown)
			{
				OpenProject();
				return true;
			}
			break;

		case Key::S:
			if (ctrlDown)
			{
				if (shiftDown)
				{
					// SaveSceneAs();
				}
				else
				{
					SaveScene();
				}
				return true;
			}
			break;
		}

		return false;
	}

	bool AppLayer::OnFileDrop(WindowFileDropEvent& e)
	{
		m_ContentBrowserPanel.OnFileDrop(e.GetPaths());
		return true;
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
		if (AssetManager::GetAssetType(m_ActiveSceneHandle) != AssetType::Scene) 
		{
			GX_CORE_ERROR("Asset with handle {0} is not a scene!", static_cast<uint64_t>(m_ActiveSceneHandle));
			return;
		}

		const auto& filePath = Project::GetAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetAssetFilePath(m_ActiveSceneHandle);

		SceneSerializer serializer(m_ActiveScene);
		serializer.Serialize(filePath);
	}

	void AppLayer::OpenScene(AssetHandle handle, bool deserialize)
	{
		if(AssetManager::GetAssetType(handle) != AssetType::Scene)
		{
			GX_CORE_ERROR("Asset with handle {0} is not a scene!", static_cast<uint64_t>(handle));
			return;
		}

		// Try to get the scene asset (may return nullptr if loading asynchronously)
		Ref<Scene> scene = AssetManager::GetAsset<Scene>(handle);

		// Only update the active scene if we successfully loaded it
		// If it's nullptr (async loading), keep the current scene
		if (scene)
		{
			m_ActiveSceneHandle = handle;
			m_ActiveScene = scene;
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportPanel.GetViewportSize().x, (uint32_t)m_ViewportPanel.GetViewportSize().y);
			m_SceneHierarchyPanel.SetContext(m_ActiveScene);
			m_SceneHierarchyPanel.SetNoneSelected();

			if(!deserialize)
				return;

			const auto& filePath = Project::GetAssetDirectory() / Project::GetActive()->GetEditorAssetManager()->GetAssetFilePath(m_ActiveSceneHandle);
			SceneSerializer serializer(m_ActiveScene);
			serializer.Deserialize(filePath);
		}
		else
		{
			// Scene is loading asynchronously, track it so we can auto-load when ready
			m_PendingSceneHandle = handle;
			GX_CORE_INFO("Scene {0} is loading asynchronously, will auto-switch when ready", static_cast<uint64_t>(handle));
		}
	}

}