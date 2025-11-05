#pragma once

#include "Core/UUID.h"

#include "SceneCamera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Gravix
{

	struct TagComponent
	{
		std::string Name;
		UUID ID;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& name, UUID uuid)
			: Name(name), ID(uuid) {}

		operator std::string&() { return Name; }
		operator const std::string&() const { return Name; }

		operator UUID&() { return ID; }
		operator const UUID&() const { return ID; }
	};

	struct TransformComponent
	{
		glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

		glm::mat4 Transform;

		TransformComponent()  { CalculateTransform(); }
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& position, const glm::vec3 rotation, const glm::vec3 scale)
			: Position(position), Rotation(rotation), Scale(scale) { CalculateTransform(); }

		void CalculateTransform() 
		{
			Transform = glm::translate(glm::mat4(1.0f), Position)
				* glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.x), { 1.0f, 0.0f, 0.0f })
				* glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.y), { 0.0f, 1.0f, 0.0f })
				* glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.z), { 0.0f, 0.0f, 1.0f })
				* glm::scale(glm::mat4(1.0f), Scale);
		}

		operator glm::mat4&() { return Transform; }
		operator const glm::mat4&() const { return Transform; }
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {}

		operator glm::vec4&() { return Color; }
		operator const glm::vec4&() const { return Color; }
	};

	struct CameraComponent 
	{
		SceneCamera Camera;
		bool Primary = false;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

}