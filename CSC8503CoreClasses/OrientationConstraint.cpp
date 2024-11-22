#include "OrientationConstraint.h"
#include "GameObject.h"
#include "PhysicsObject.h"
using namespace NCL;
using namespace Maths;
using namespace CSC8503;

OrientationConstraint::OrientationConstraint(GameObject* a, GameObject* b, Quaternion target, Maths::Vector3 minRotation, Vector3 maxRotation)
{
	objectA = a;
	objectB = b;
	targetOrientation = target;
	this->maxRotation = maxRotation;
}

OrientationConstraint::~OrientationConstraint()
{

}

void OrientationConstraint::UpdateConstraint(float dt) {
	PhysicsObject* physA = objectA->GetPhysicsObject();
	PhysicsObject* physB = objectB->GetPhysicsObject();

	Quaternion relativeOrientation = objectA->GetTransform().GetOrientation().Conjugate() * objectB->GetTransform().GetOrientation();
	Quaternion offset = targetOrientation * relativeOrientation.Conjugate();
	Vector3 euler = offset.ToEuler();

	Vector3 neededCorrection = getOffsetFromTarget(euler);
	if (neededCorrection == Vector3(0, 0, 0)) {
		return;
	}
	Vector3 correctionDir = Vector::Normalise(neededCorrection);

	float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();
	float bias = -(BiasFactor / dt);
	float lambda = bias / constraintMass;

	Vector3 aImpulse = correctionDir * lambda;
	Vector3 bImpulse = -correctionDir * lambda;

	// TODO: This maths is wrong, what should we do instead?
	//physA->ApplyAngularImpulse(aImpulse);
	//physB->ApplyAngularImpulse(bImpulse);
}

Vector3 OrientationConstraint::getOffsetFromTarget(const Vector3& euler) const {

	Vector3 fromMax = euler - maxRotation;
	Vector3 fromMin = euler - minRotation;

	Vector3 result;
	result.x = (fromMax.x > 0) ? fromMax.x : (fromMin.x < 0) ? fromMin.x : 0;
	result.y = (fromMax.y > 0) ? fromMax.y : (fromMin.y < 0) ? fromMin.y : 0;
	result.z = (fromMax.z > 0) ? fromMax.z : (fromMin.z < 0) ? fromMin.z : 0;
	return result;
}