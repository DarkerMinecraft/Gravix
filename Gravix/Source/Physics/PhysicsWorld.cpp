#include "pch.h"
#include "PhysicsWorld.h"

namespace Gravix
{

	PhysicsWorld::PhysicsWorld()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		auto& config = Project::GetActive()->GetConfig();
		worldDef.gravity = { config.Physics.Gravity.x, config.Physics.Gravity.y };
		worldDef.restitutionThreshold = config.Physics.RestitutionThreshold;

		m_World = b2CreateWorld(&worldDef);
	}

	PhysicsWorld::~PhysicsWorld()
	{
		for (auto& id : m_Shapes)
			b2DestroyShape(id, false);

		for (auto& id : m_Bodies)
			b2DestroyBody(id);

		b2DestroyWorld(m_World);
	}

	void PhysicsWorld::Step(float timeStep, int subStepCount)
	{
		b2World_Step(m_World, timeStep, subStepCount);
	}

	uint64_t PhysicsWorld::CreateBody(const TransformComponent& transform, const Rigidbody2DComponent& rb2d)
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		switch (rb2d.Type)
		{
		case Rigidbody2DComponent::BodyType::Static:
			bodyDef.type = b2BodyType::b2_staticBody;
			break;
		case Rigidbody2DComponent::BodyType::Dynamic:
			bodyDef.type = b2BodyType::b2_dynamicBody;
			break;
		case Rigidbody2DComponent::BodyType::Kinematic:
			bodyDef.type = b2BodyType::b2_kinematicBody;
			break;
		}

		bodyDef.position = b2Vec2{ transform.Position.x, transform.Position.y };
		bodyDef.rotation = b2MakeRot(glm::radians(transform.Rotation.z));
		bodyDef.motionLocks.angularZ = rb2d.FixedRotation;

		b2BodyId bodyId = b2CreateBody(m_World, &bodyDef);
		m_Bodies.push_back(bodyId);
		return b2StoreBodyId(bodyId);

	}

	uint64_t PhysicsWorld::CreateBoxShape(uint64_t bodyId, const TransformComponent& transform, const BoxCollider2DComponent& boxCollider)
	{
		b2Polygon box = b2MakeBox(boxCollider.Size.x * transform.Scale.x, boxCollider.Size.y * transform.Scale.y);

		b2ShapeDef boxShape = b2DefaultShapeDef();
		boxShape.density = boxCollider.Density;
		boxShape.material.friction = boxCollider.Friction;
		boxShape.material.restitution = boxCollider.Restitution;

		b2ShapeId shapeId = b2CreatePolygonShape(b2LoadBodyId(bodyId), &boxShape, &box);
		m_Shapes.push_back(shapeId);
		return b2StoreShapeId(shapeId);
	}

	glm::vec2 PhysicsWorld::GetBodyPosition(uint64_t bodyId)
	{
		b2Vec2 pos = b2Body_GetPosition(b2LoadBodyId(bodyId));

		return glm::vec2{ pos.x, pos.y };
	}

	float PhysicsWorld::GetBodyRotation(uint64_t bodyId)
	{
		b2Rot rot = b2Body_GetRotation(b2LoadBodyId(bodyId));
		return glm::degrees(b2Rot_GetAngle(rot));
	}

}