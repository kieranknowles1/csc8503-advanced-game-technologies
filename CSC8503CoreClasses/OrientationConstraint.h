#pragma once
#include "Constraint.h"

#include "Quaternion.h"

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		class OrientationConstraint : public Constraint
		{
		public:
			OrientationConstraint(GameObject* a, GameObject* b, Maths::Quaternion target, Maths::Vector3 minRotation, Maths::Vector3 maxRotation);
			~OrientationConstraint();

			void UpdateConstraint(float dt) override;

		protected:
			GameObject* objectA;
			GameObject* objectB;

			// The target orientation of objectB relative to objectA
			Maths::Quaternion targetOrientation;
			// The maximum amount objectB can be rotated relative to objectA, in each axis
			Maths::Vector3 minRotation;
			Maths::Vector3 maxRotation;

			// How far are we from the target limits?
			Maths::Vector3 getOffsetFromTarget(const Maths::Vector3& euler) const;
		};
	}
}

