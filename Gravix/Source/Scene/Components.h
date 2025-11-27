#pragma once

#include "Core/UUID.h"

#include "SceneCamera.h"

#include "Asset/Asset.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <typeindex>

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
			: Name(name), ID(uuid), CreationIndex(creationIndex) {
		}

		operator std::string& () { return Name; }
		operator const std::string& () const { return Name; }

		operator UUID& () { return ID; }
		operator const UUID& () const { return ID; }
	};

	struct TransformComponent
	{
		glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

		glm::mat4 Transform;

		TransformComponent() { CalculateTransform(); }
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& position, const glm::vec3 rotation, const glm::vec3 scale)
			: Position(position), Rotation(rotation), Scale(scale) {
			CalculateTransform();
		}

		void CalculateTransform()
		{
			glm::vec3 radianRotation = { glm::radians(Rotation.x), glm::radians(Rotation.y), glm::radians(Rotation.z) };
			glm::mat4 rotation = glm::toMat4(glm::quat(radianRotation));

			Transform = glm::translate(glm::mat4(1.0f), Position)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}

		operator glm::mat4& () { return Transform; }
		operator const glm::mat4& () const { return Transform; }
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		AssetHandle Texture = 0;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color, const AssetHandle handle, float tilingFactor)
			: Color(color), Texture(handle), TilingFactor(tilingFactor) {
		}

		operator glm::vec4& () { return Color; }
		operator const glm::vec4& () const { return Color; }

		operator AssetHandle& () { return Texture; }
		operator const AssetHandle& () const { return Texture; }

		operator float& () { return TilingFactor; }
		operator const float& () const { return TilingFactor; }
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.005f;

		CircleRendererComponent() = default;
		CircleRendererComponent(const CircleRendererComponent&) = default;
		CircleRendererComponent(const glm::vec4& color, float thickness, float fade)
			: Color(color), Thickness(thickness), Fade(fade) {
		}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = false;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct ScriptComponent
	{
		std::string Name;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;
	};

	struct Rigidbody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		// Runtime physics body stored as uint64_t (physics system converts to/from Box2D IDs)
		uint64_t RuntimeBody = 0;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
		Rigidbody2DComponent(BodyType type, bool fixedRotation)
			: Type(type), FixedRotation(fixedRotation) {
		}
	};

	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.5f, 0.5f };

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;

		// Runtime physics shape stored as uint64_t (physics system converts to/from Box2D IDs)
		uint64_t RuntimeShape = 0;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
		BoxCollider2DComponent(const glm::vec2& offset, const glm::vec2& size, float density, float friction, float restitution)
			: Offset(offset), Size(size), Density(density), Friction(friction), Restitution(restitution) {
		}
	};

	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 1.0f, 1.0f };

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;

		// Runtime physics shape stored as uint64_t (physics system converts to/from Box2D IDs)
		uint64_t RuntimeShape = 0;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
		CircleCollider2DComponent(const glm::vec2& offset, const glm::vec2& size, float density, float friction, float restitution)
			: Offset(offset), Size(size), Density(density), Friction(friction), Restitution(restitution) {
		}
	};

	// Hidden component that tracks the order components were added to an entity
	struct ComponentOrderComponent
	{
		std::vector<std::type_index> ComponentOrder;

		ComponentOrderComponent() = default;
		ComponentOrderComponent(const ComponentOrderComponent&) = default;
	};

}