#pragma once

#include <glm/glm.hpp>

namespace Gravix 
{

	class Renderer2D 
	{
	public:
		static void Init();

		static void BeginScene(const glm::mat4& viewProjection);
		static void EndScene();
		static void Flush();

		static void Destroy();
	};

}