#include "ProjectSettingsPanel.h"
#include "Utils/PlatformUtils.h"

#include <imgui.h>
#include <cstring>

namespace Gravix
{

	void ProjectSettingsPanel::OnImGuiRender()
	{
		if (!m_IsOpen)
			return;

		if (!Project::HasActiveProject())
		{
			m_IsOpen = false;
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Project Settings", &m_IsOpen))
		{
			auto& config = Project::GetActive()->GetConfig();

			// Initialize buffers on first open
			static bool initialized = false;
			if (!initialized)
			{
				strncpy(m_ProjectNameBuffer, config.Name.c_str(), sizeof(m_ProjectNameBuffer) - 1);
				strncpy(m_AssetDirectoryBuffer, config.AssetDirectory.string().c_str(), sizeof(m_AssetDirectoryBuffer) - 1);
				strncpy(m_LibraryDirectoryBuffer, config.LibraryDirectory.string().c_str(), sizeof(m_LibraryDirectoryBuffer) - 1);
				strncpy(m_ScriptPathBuffer, config.ScriptPath.string().c_str(), sizeof(m_ScriptPathBuffer) - 1);
				initialized = true;
			}

			// Reload button to refresh from current config
			if (ImGui::Button("Reload"))
			{
				strncpy(m_ProjectNameBuffer, config.Name.c_str(), sizeof(m_ProjectNameBuffer) - 1);
				strncpy(m_AssetDirectoryBuffer, config.AssetDirectory.string().c_str(), sizeof(m_AssetDirectoryBuffer) - 1);
				strncpy(m_LibraryDirectoryBuffer, config.LibraryDirectory.string().c_str(), sizeof(m_LibraryDirectoryBuffer) - 1);
				strncpy(m_ScriptPathBuffer, config.ScriptPath.string().c_str(), sizeof(m_ScriptPathBuffer) - 1);
			}

			ImGui::Separator();
			ImGui::Spacing();

			// Project Name
			ImGui::Text("Project Name");
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##ProjectName", m_ProjectNameBuffer, sizeof(m_ProjectNameBuffer));
			ImGui::Spacing();

			// Asset Directory
			ImGui::Text("Asset Directory");
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##AssetDirectory", m_AssetDirectoryBuffer, sizeof(m_AssetDirectoryBuffer));
			ImGui::Spacing();

			// Library Directory
			ImGui::Text("Library Directory");
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##LibraryDirectory", m_LibraryDirectoryBuffer, sizeof(m_LibraryDirectoryBuffer));
			ImGui::Spacing();

			// Script Path
			ImGui::Text("Script Path");
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##ScriptPath", m_ScriptPathBuffer, sizeof(m_ScriptPathBuffer));
			ImGui::Spacing();

			ImGui::Separator();
			ImGui::Spacing();

			// Start Scene
			ImGui::Text("Start Scene Handle: %llu", static_cast<uint64_t>(config.StartScene));
			ImGui::Spacing();

			ImGui::Separator();
			ImGui::Spacing();

			// Save button
			if (ImGui::Button("Save", ImVec2(120, 0)))
			{
				// Update config
				config.Name = m_ProjectNameBuffer;
				config.AssetDirectory = m_AssetDirectoryBuffer;
				config.LibraryDirectory = m_LibraryDirectoryBuffer;
				config.ScriptPath = m_ScriptPathBuffer;

				// Save project - we need the project path
				// For now, we'll just update the config in memory
				// The actual save will happen when the user saves the project normally
				GX_CORE_INFO("Project settings updated");
			}

			ImGui::SameLine();

			// Close button
			if (ImGui::Button("Close", ImVec2(120, 0)))
			{
				m_IsOpen = false;
			}
		}
		ImGui::End();

		// Reset initialization flag when panel is closed
		if (!m_IsOpen)
		{
			static bool initialized = true;
			initialized = false;
		}
	}

}
