#pragma once
#include "Constraint.h"

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		class PositionConstraint : public Constraint	{
		public:
			enum class Type {
				// Push and pull, like a metal rod
				Rigid,
				// Only pull, like a rope
				Rope,
				// Only push, less realistic, think of Star Wars hover cars
				Repulse
			};

			PositionConstraint(GameObject* a, GameObject* b, float d, Type type = Type::Rigid);
			~PositionConstraint();

			void UpdateConstraint(float dt) override;

		protected:
			GameObject* objectA;
			GameObject* objectB;

			Type type;
			float distance;

			// Are we outside of our target distance, given our type
			// and offset from the target distance?
			// Pass targetDistance - currentDistance
			bool isOutsideDistance(float targetOffset) const;
		};
	}
}