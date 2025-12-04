#include "Core/Application.h"
#include "AppLayer.h"


#ifdef ENGINE_PLATFORM_WINDOWS
#ifdef ENGINE_DEBUG

#include "Core/Log.h"
#include "Debug/Instrumentor.h"

	int main()
	{

		Gravix::Log::Init();

		// === STARTUP PROFILING ===
		GX_PROFILE_BEGIN_SESSION("Startup", "Gravix-Profile-Startup.json");

		Gravix::ApplicationSpecification appSpec{};
		appSpec.Width = 1280;
		appSpec.Height = 720;
		appSpec.Title = "Orbit";

		// Optional: Load a default project on startup
		// Gravix::Project::Load("C:/Dev/Orbit/Testing/Testing.orbproj");

		Gravix::Application app(appSpec);
		app.PushLayer<Gravix::AppLayer>();

		GX_PROFILE_END_SESSION();

		// === RUNNING PROFILING ===
		GX_PROFILE_BEGIN_SESSION("Runtime", "Gravix-Profile-Runtime.json");
		app.Run();
		GX_PROFILE_END_SESSION();

		// === SHUTDOWN PROFILING ===
		GX_PROFILE_BEGIN_SESSION("Shutdown", "Gravix-Profile-Shutdown.json");

		// Destructor will be called here

		GX_PROFILE_END_SESSION();

		return 0;
	}

#endif
#ifdef ENGINE_RELEASE

	#include "SplashScreen.h"
	#include "Project/Project.h"
	#include "Utils/PlatformUtils.h"
	#include <shlobj.h>

	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
	{
		// Initialize COM for folder dialog
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		// Create and show splash screen
		Gravix::SplashScreen splash("Orbit Editor");
		splash.Show();
		splash.SetStatus("Initializing Orbit...");
		splash.SetProgress(0.05f);

		try
		{
			// Parse command line for project path
			std::filesystem::path projectPath;
			if (pCmdLine && wcslen(pCmdLine) > 0)
			{
				// Remove quotes if present
				std::wstring wPath = pCmdLine;
				if (!wPath.empty() && wPath.front() == L'"')
					wPath = wPath.substr(1, wPath.length() - 2);

				// Convert to std::filesystem::path
				projectPath = wPath;
			}
			else
			{
				// No command line arg - show folder picker
				splash.SetStatus("Select project folder...");

				// Use FileDialogs with splash window as owner
				projectPath = Gravix::FileDialogs::OpenFolderWithOwner(splash.GetWindowHandle(), "Select Project Folder");

				if (projectPath.empty())
				{
					splash.Close();
					CoUninitialize();
					return 0; // User cancelled
				}
			}

			splash.SetStatus("Initializing rendering system...");
			splash.SetProgress(0.15f);

			// Create application (window hidden initially)
			Gravix::ApplicationSpecification appSpec{};
			appSpec.Width = 1280;
			appSpec.Height = 720;
			appSpec.Title = "Orbit";

			Gravix::Application app(appSpec);

			// Window is already hidden in Release mode - no need to call Hide()

			splash.SetStatus("Loading project configuration...");
			splash.SetProgress(0.3f);

			// Load or create project
			if (!projectPath.empty() && std::filesystem::is_directory(projectPath))
			{
				// Look for any .orbproj file in the directory
				std::filesystem::path projectFilePath = projectPath / ".orbproj";
				bool foundProject = std::filesystem::exists(projectFilePath);

				if (foundProject)
				{
					// Load existing project
					Gravix::Project::Load(projectFilePath.string());
				}
				else
				{
					Gravix::Project::New(projectPath);
					Gravix::Project::SaveActive(projectFilePath);
				}
			}

			splash.SetStatus("Initializing scripting engine...");
			splash.SetProgress(0.5f);

			splash.SetStatus("Loading editor assembly...");
			splash.SetProgress(0.65f);

			// Push editor layer
			app.PushLayer<Gravix::AppLayer>();

			splash.SetStatus("Importing assets...");
			splash.SetProgress(0.8f);

			splash.SetStatus("Finalizing...");
			splash.SetProgress(0.95f);

			splash.SetStatus("Ready!");
			splash.SetProgress(1.0f);

			// Small delay to show "Ready!" message
			Sleep(100);

			// Close splash first
			splash.Close();

			// Now show main window (fixes prevent it from minimizing)
			app.GetWindow().Show();

			// Run application
			app.Run();

			CoUninitialize();
			return 0;
		}
		catch (const std::exception& e)
		{
			splash.Close();

			// Show error message
			std::string errorMsg = "Orbit Editor failed to start:\n\n";
			errorMsg += e.what();

			MessageBoxA(nullptr, errorMsg.c_str(), "Orbit Editor Error", MB_OK | MB_ICONERROR);

			CoUninitialize();
			return 1;
		}
		catch (...)
		{
			splash.Close();
			MessageBoxA(nullptr, "Orbit Editor failed to start due to an unknown error.", "Orbit Editor Error", MB_OK | MB_ICONERROR);

			CoUninitialize();
			return 1;
		}
	}

#endif 
#endif