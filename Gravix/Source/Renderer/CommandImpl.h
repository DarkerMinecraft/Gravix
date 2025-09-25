#pragma once

#include <glm/glm.hpp>

namespace Gravix 
{

	class CommandImpl 
	{
	public:
		virtual ~CommandImpl() = default;
	
		virtual void BeginRendering() = 0;
		virtual void DrawImGui() = 0;
		virtual void EndRendering() = 0;

		virtual void CopyToSwapchain() = 0;
	};

}