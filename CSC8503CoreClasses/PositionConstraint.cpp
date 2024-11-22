#include "PositionConstraint.h"
//#include "../../Common/Vector3.h"
#include "GameObject.h"
#include "PhysicsObject.h"
#include "Debug.h"



using namespace NCL;
using namespace Maths;
using namespace CSC8503;

PositionConstraint::PositionConstraint(GameObject* a, GameObject* b, float d)
{
	objectA		= a;
	objectB		= b;
	distance	= d;
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

	Debug::DrawLine(objectA->GetTransform().GetPosition(), objectB->GetTransform().GetPosition(), offset > 0 ? Vector4(0, 1, 0, 1) : Vector4(1, 0, 0, 1));

	// TODO: Enum for type. Rope, rigid, or repulse
	if (offset < 0.0f) {
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
