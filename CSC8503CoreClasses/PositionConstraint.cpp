#include "PositionConstraint.h"

#include <cassert>

#include "GameObject.h"
#include "PhysicsObject.h"
#include "Debug.h"



using namespace NCL;
using namespace Maths;
using namespace CSC8503;

PositionConstraint::PositionConstraint(GameObject* a, GameObject* b, float d, Type type) :
		objectA(a), objectB(b), distance(d), type(type)
{
}

PositionConstraint::~PositionConstraint()
{

}

//a simple constraint that stops objects from being more than <distance> away
//from each other...this would be all we need to simulate a rope, or a ragdoll
void PositionConstraint::UpdateConstraint(float dt)	{
	Vector3 relativePosition =
		objectA->GetTransform().GetPosition() -
		objectB->GetTransform().GetPosition();

	float currentDistance = Vector::Length(relativePosition);

	// How far are we from our desired distance?
	float offset = distance - currentDistance;

	bool needsCorrection = isOutsideDistance(offset);
	Debug::DrawLine(
		objectA->GetTransform().GetPosition(),
		objectB->GetTransform().GetPosition(),
		needsCorrection ? Debug::RED : Debug::GREEN
	);

	if (needsCorrection) {
		Vector3 offsetDir = Vector::Normalise(relativePosition);
		PhysicsObject* physA = objectA->GetPhysicsObject();
		PhysicsObject* physB = objectB->GetPhysicsObject();

		Vector3 relativeVelocity = physA->GetLinearVelocity() - physB->GetLinearVelocity();

		float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();
		if (constraintMass > 0.0f) {
			float velocityDot = Vector::Dot(relativeVelocity, offsetDir);
			float bias = -(BiasFactor / dt) * offset;

			float lambda = -(bias + velocityDot) / constraintMass;
			Vector3 aImpulse = offsetDir * lambda;
			Vector3 bImpulse = -offsetDir * lambda;

			// These get multiplied by the inverse mass
			physA->ApplyLinearImpulse(aImpulse);
			physB->ApplyLinearImpulse(bImpulse);
		}
	}
}

bool PositionConstraint::isOutsideDistance(float targetOffset) const {
	switch (type)
	{
	case Type::Rigid:
		return targetOffset != 0.0f;
	case Type::Rope:
		return targetOffset < 0.0f; // Distance > target distance
	case Type::Repulse:
		return targetOffset > 0.0f; // Distance < target distance
	default:
		assert(false); // Invalid type
	}
}