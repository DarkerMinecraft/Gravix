#pragma once

#include "Core/RefCounted.h"
#include "Scene/Components.h"

#include "Project/Project.h"

#include <glm/glm.hpp>
#include <box2d/box2d.h>

namespace Gravix
{

	class PhysicsWorld : public RefCounted
	{
	public:
		PhysicsWorld();
		~PhysicsWorld();

		void Step(float timeStep, int subStepCount);

		uint64_t CreateBody(const TransformComponent& transform, const Rigidbody2DComponent& rb2d);

		uint64_t CreateBoxShape(uint64_t bodyId, const TransformComponent& transform, const BoxCollider2DComponent& boxCollider);
		uint64_t CreateCircleShape(uint64_t bodyId, const TransformComponent& transform, const CircleCollider2DComponent& circleCollider);

		void ApplyLinearImpulse(uint64_t bodyId, const glm::vec2& impulse, const glm::vec2& point, bool wake);
		void ApplyLinearImpulseToCenter(uint64_t bodyId, const glm::vec2& impulse, bool wake);

		void ApplyForce(uint64_t bodyID, const glm::vec2& force, const glm::vec2& point, bool wake);
		void ApplyForceToCenter(uint64_t bodyID, const glm::vec2& force, bool wake);
		
		glm::vec2 GetBodyPosition(uint64_t bodyId);
		float GetBodyRotation(uint64_t bodyId);
	private:
		b2WorldId m_World;

		std::vector<b2BodyId> m_Bodies;
		std::vector<b2ShapeId> m_Shapes;
	};

}