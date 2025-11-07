#include "Application.h"
#include "AppLayer.h"


#ifdef ENGINE_PLATFORM_WINDOWS
#ifdef ENGINE_DEBUG

#include "Log.h"
#include "Project/Project.h"
#include <crtdbg.h>

	int main()
	{

		//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

		Gravix::Log::Init();

		Gravix::ApplicationSpecification appSpec{};
		appSpec.Width = 1280;
		appSpec.Height = 720;
		appSpec.Title = "Orbit";

		Gravix::Project::Load("C:/Dev/Orbit/Testing/Testing.orbproj");

		Gravix::Application app(appSpec);
		app.PushLayer<Gravix::AppLayer>();
		app.Run();

		return 0;
	}

#endif
#ifdef ENGINE_RELEASE

	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
	{
		Gravix::ApplicationSpecification appSpec{};
		appSpec.Width = 1280;
		appSpec.Height = 720;
		appSpec.Title = "Orbit";

		Gravix::Application app(appSpec);
		app.PushLayer<Gravix::AppLayer>();
		app.Run();

		return 0;
	}

#endif
#endif 
