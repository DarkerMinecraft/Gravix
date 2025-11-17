#pragma once

#include "Renderer/Generic/Command.h"
#include "Renderer/Generic/Types/Texture.h"

#include "Scene/EditorCamera.h"
#include "Scene/SceneCamera.h"

#include <glm/glm.hpp>

namespace Gravix
{
	
	class Renderer2D
	{
	public:
		static void Init(Ref<Framebuffer> renderTarget);

		static void BeginScene(Command& cmd, EditorCamera& camera);
		static void BeginScene(Command& cmd, Camera& camera, const glm::mat4& transformationMatrix);

		static void DrawQuad(const glm::mat4& transformMatrix, uint32_t entityID, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, Ref<Texture2D> texture = nullptr, float tilingFactor = 1.0f);
		static void DrawCircle(const glm::mat4& transformMatrix, uint32_t entityID, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, float thickness = 0.1f, float fade = 0.005f);

		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void DrawQuadOutline(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
		static void DrawQuadOutline(const glm::mat4& transformMatrix, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void DrawCircleOutline(const glm::mat3& position, const glm::vec2& size, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
		static void DrawCircleOutline(const glm::mat4& transformMatrix, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void EndScene(Command& cmd);
		static void Flush(Command& cmd);

		static void Destroy();
	};

}