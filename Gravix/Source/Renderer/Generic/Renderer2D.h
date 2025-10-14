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

		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) { DrawQuad(glm::vec3(position.x, position.y, 0), size, color); };
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f)) { DrawQuad(glm::vec3(position.x, position.y, 0), size, texture, tintColor); };
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void EndScene(Command& cmd);
		static void Flush(Command& cmd);

		static void Destroy();
	};

}