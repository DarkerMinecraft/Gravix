#pragma once

#include "Renderer/Generic/Command.h"
#include "Renderer/Generic/Texture.h"

#include <glm/glm.hpp>

namespace Gravix 
{

	struct QuadVertex 
	{
		glm::vec3 Position;
		glm::vec2 Size;
		float Rotation;

		glm::vec4 Color;
		Ref<Texture2D> Texture = nullptr;
		float TilingFactor = 1;
	};

	class Renderer2D 
	{
	public:
		static void Init(Ref<Framebuffer> renderTarget);

		static void BeginScene(Command& cmd, const glm::mat4& viewProjection);

		static void DrawQuad(const QuadVertex& quadVertex);

		static void EndScene(Command& cmd);
		static void Flush(Command& cmd);

		static void Destroy();
	};

}