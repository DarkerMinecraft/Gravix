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
		GX_PROFILE_BEGIN_SESSION("Startup", "Gravix-Runtime-Profile-Startup.json");

		Gravix::ApplicationSpecification appSpec{};
		appSpec.Width = 1280;
		appSpec.Height = 720;
		appSpec.Title = "Gravix Runtime";
		appSpec.IsRuntime = true;

		Gravix::Application app(appSpec);
		app.PushLayer<Gravix::AppLayer>();

		GX_PROFILE_END_SESSION();

		// === RUNNING PROFILING ===
		GX_PROFILE_BEGIN_SESSION("Runtime", "Gravix-Runtime-Profile-Runtime.json");
		app.Run();
		GX_PROFILE_END_SESSION();

		// === SHUTDOWN PROFILING ===
		GX_PROFILE_BEGIN_SESSION("Shutdown", "Gravix-Runtime-Profile-Shutdown.json");

		// Destructor will be called here

		GX_PROFILE_END_SESSION();

		return 0;
	}

#endif
#ifdef ENGINE_RELEASE

	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
	{
		Gravix::ApplicationSpecification appSpec{};
		appSpec.Width = 1280;
		appSpec.Height = 720;
		appSpec.Title = "Gravix Runtime";

		Gravix::Application app(appSpec);
		app.PushLayer<Gravix::AppLayer>();
		app.Run();

		return 0;
	}

#endif
#endif
