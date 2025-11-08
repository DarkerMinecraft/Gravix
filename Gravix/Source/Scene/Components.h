#pragma once

#include "Core/UUID.h"

#include "SceneCamera.h"

#include "Asset/Asset.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Gravix
{

	struct TagComponent
	{
		std::string Name;
		UUID ID;
		uint32_t CreationIndex = 0;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& name, UUID uuid, uint32_t creationIndex = 0)
			: Name(name), ID(uuid), CreationIndex(creationIndex) {}

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
			glm::vec3 radianRotation = {glm::radians(Rotation.x), glm::radians(Rotation.y), glm::radians(Rotation.z)};
			glm::mat4 rotation = glm::toMat4(glm::quat(radianRotation));

			Transform = glm::translate(glm::mat4(1.0f), Position)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}

		operator glm::mat4&() { return Transform; }
		operator const glm::mat4&() const { return Transform; }
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		AssetHandle Texture = 0;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color, const AssetHandle handle, float tilingFactor)
			: Color(color), Texture(handle), TilingFactor(tilingFactor) {}

		operator glm::vec4&() { return Color; }
		operator const glm::vec4&() const { return Color; }

		operator AssetHandle& () { return Texture; }
		operator const AssetHandle& () const { return Texture; }

		operator float& () { return TilingFactor; }
		operator const float& () const { return TilingFactor; }
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