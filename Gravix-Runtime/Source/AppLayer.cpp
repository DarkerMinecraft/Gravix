#include "AppLayer.h"

namespace Gravix
{

	AppLayer::AppLayer()
		: Layer()
	{
		
	}

	AppLayer::~AppLayer()
	{
	}

	void AppLayer::OnEvent(Event& event)
	{
		
	}

	void AppLayer::OnUpdate(float deltaTime)
	{
		// TODO: Update runtime systems
		// - Update scene
		// - Update physics
		// - Update scripting
	}

	void AppLayer::OnRender()
	{
		// TODO: Render game scene
		// - Render active scene to framebuffer
		// - Post-processing
		// - Present to screen
	
	}

}
