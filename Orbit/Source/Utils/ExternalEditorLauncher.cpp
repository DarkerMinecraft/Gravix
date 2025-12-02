#include "ExternalEditorLauncher.h"

#include "Core/Log.h"
#include "Project/Project.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gravix
{

	bool ExternalEditorLauncher::LaunchProcess(const std::string& command)
	{
#ifdef _WIN32
		STARTUPINFOA si = { sizeof(si) };
		PROCESS_INFORMATION pi;

		if (CreateProcessA(
			NULL,
			const_cast<char*>(command.c_str()),
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&si,
			&pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return true;
		}

		GX_CORE_ERROR("Failed to launch external editor");
		return false;
#else
		return system(command.c_str()) == 0;
#endif
	}

	bool ExternalEditorLauncher::OpenScript(const std::filesystem::path& scriptPath)
	{
		auto& config = Project::GetActive()->GetConfig();

		// Validate script editor configuration
		if (config.ScriptEditorPath.empty())
		{
			GX_CORE_WARN("No external script editor configured. Please set one in Project Settings.");
			return false;
		}

		if (!std::filesystem::exists(config.ScriptEditorPath))
		{
			GX_CORE_ERROR("Script editor not found at: {0}", config.ScriptEditorPath.string());
			return false;
		}

		// Try to find and open the project file
		std::filesystem::path csprojPath = config.ScriptPath / (config.Name + ".csproj");

		std::string command;
		if (std::filesystem::exists(csprojPath))
		{
			command = "\"" + config.ScriptEditorPath.string() + "\" \"" + csprojPath.string() + "\"";
			GX_CORE_INFO("Opening project: {0}", csprojPath.filename().string());
		}
		else
		{
			command = "\"" + config.ScriptEditorPath.string() + "\" \"" + scriptPath.string() + "\"";
			GX_CORE_WARN("Project file not found: {0}. Opening script file directly.", csprojPath.string());
		}

		return LaunchProcess(command);
	}

	bool ExternalEditorLauncher::OpenProject(const std::filesystem::path& projectPath)
	{
		auto& config = Project::GetActive()->GetConfig();

		// Validate script editor configuration
		if (config.ScriptEditorPath.empty())
		{
			GX_CORE_WARN("No external script editor configured. Please set one in Project Settings.");
			return false;
		}

		if (!std::filesystem::exists(config.ScriptEditorPath))
		{
			GX_CORE_ERROR("Script editor not found at: {0}", config.ScriptEditorPath.string());
			return false;
		}

		if (!std::filesystem::exists(projectPath))
		{
			GX_CORE_ERROR("Project file not found at: {0}", projectPath.string());
			return false;
		}

		std::string command = "\"" + config.ScriptEditorPath.string() + "\" \"" + projectPath.string() + "\"";
		GX_CORE_INFO("Opening project: {0}", projectPath.filename().string());

		return LaunchProcess(command);
	}

}
