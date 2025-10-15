#pragma once

#include "Renderer/Generic/Command.h"
#include "Renderer/Generic/Texture.h"

#include <glm/glm.hpp>

namespace Gravix
{

	class Renderer2D
	{
	public:
		static void Init(Ref<Framebuffer> renderTarget);

		static void BeginScene(Command& cmd, const glm::mat4& viewProjection);

		static void DrawQuad(const glm::mat4& transformMatrix, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, Ref<Texture2D> texture = nullptr, float tilingFactor = 1.0f);

		static void EndScene(Command& cmd);
		static void Flush(Command& cmd);

		static void Destroy();
	};

}