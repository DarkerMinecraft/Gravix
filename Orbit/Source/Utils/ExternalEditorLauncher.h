#pragma once

#include <filesystem>

namespace Gravix
{

	class ExternalEditorLauncher
	{
	public:
		// Open a script file with the configured external editor
		static bool OpenScript(const std::filesystem::path& scriptPath);

		// Open a C# project file with the configured external editor
		static bool OpenProject(const std::filesystem::path& projectPath);

	private:
		static bool LaunchProcess(const std::string& command);
	};

}
