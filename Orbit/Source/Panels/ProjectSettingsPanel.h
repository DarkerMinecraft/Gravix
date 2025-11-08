#pragma once

#include "Gravix.h"
#include <string>

namespace Gravix
{

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
		bool m_IsOpen = false;

		// Temporary buffers for editing
		char m_ProjectNameBuffer[256] = "";
		char m_AssetDirectoryBuffer[512] = "";
		char m_LibraryDirectoryBuffer[512] = "";
		char m_ScriptPathBuffer[512] = "";
	};

}
