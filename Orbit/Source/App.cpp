#include "Core/Application.h"
#include "Core/Log.h"

#include "AppLayer.h"

#ifdef ENGINE_PLATFORM_WINDOWS
#ifdef ENGINE_DEBUG

	int main()
	{
		Gravix::Log::Init();

		Gravix::ApplicationSpecification appSpec;
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
