#pragma once

namespace Gravix
{

	enum class ProjectSettingsTab
	{
		General = 0,
		Physics
	};

	class ProjectSettingsPanel
	{
	public:
		ProjectSettingsPanel() = default;
		~ProjectSettingsPanel() = default;

		void Open() { m_IsOpen = true; }
		void Close() { m_IsOpen = false; }
		bool IsOpen() const { return m_IsOpen; }

		void OnImGuiRender();
	private:
		void RenderGeneralTab();
		void RenderPhysicsTab();

		bool m_IsOpen = false;
		ProjectSettingsTab m_CurrentTab = ProjectSettingsTab::General;

		// Temporary buffers for editing
		char m_ProjectNameBuffer[256] = "";
		char m_AssetDirectoryBuffer[512] = "";
		char m_LibraryDirectoryBuffer[512] = "";
		char m_ScriptPathBuffer[512] = "";
		char m_ScriptEditorPathBuffer[512] = "";
	};

}
