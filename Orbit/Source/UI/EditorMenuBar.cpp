#include "EditorMenuBar.h"

#include "ProjectManager.h"
#include "SceneManager.h"
#include "Panels/ProjectSettingsPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Core/Application.h"
#include "Debug/ProfilerViewer.h"

#include <imgui.h>

namespace Gravix
{

	void EditorMenuBar::OnImGuiRender()
	{
		if (!ImGui::BeginMenuBar())
			return;

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl+N"))
			{
				if (m_ProjectManager && m_ProjectManager->CreateNewProject())
				{
					if (m_OnProjectCreated)
						m_OnProjectCreated();
				}
			}

			if (ImGui::MenuItem("Open... ", "Ctrl+O"))
			{
				if (m_ProjectManager && m_ProjectManager->OpenProject())
				{
					if (m_OnProjectOpened)
						m_OnProjectOpened();
				}
			}

			if (m_SceneManager && ImGui::MenuItem("Save", "Ctrl+S"))
			{
				m_SceneManager->SaveActiveScene();
			}

			if (m_SceneManager && ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
			{
				// TODO: Implement SaveSceneAs
			}

			ImGui::Separator();

#ifdef ENGINE_DEBUG
			// Toggle profiler viewer
			bool profilerVisible = Application::Get().GetProfiler().IsVisible();
			if (ImGui::MenuItem("Profiler Viewer", nullptr, &profilerVisible))
			{
				Application::Get().GetProfiler().SetVisible(profilerVisible);
			}

			ImGui::Separator();
#endif

			if (m_ProjectSettingsPanel && ImGui::MenuItem("Preferences..."))
			{
				m_ProjectSettingsPanel->Open();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

}
