#include "Application.h"
#include "Log.h"

#include "AppLayer.h"

#include <crtdbg.h>

#ifdef ENGINE_PLATFORM_WINDOWS
#ifdef ENGINE_DEBUG

	int main()
	{

		//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

		Gravix::Log::Init();

		Gravix::ApplicationSpecification appSpec{};
		appSpec.Width = 1280;
		appSpec.Height = 720;
		appSpec.Title = "Orbit";

		Gravix::Application app(appSpec);
		app.PushLayer<Orbit::AppLayer>();
		app.Run();

		return 0;
	}

#endif
#endif 
