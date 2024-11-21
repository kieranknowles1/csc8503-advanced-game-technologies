#pragma once

namespace NCL {
	namespace CSC8503 {
		class Constraint	{
		public:
			constexpr static float BiasFactor = 0.01f;

			Constraint() {}
			virtual ~Constraint() {}

			virtual void UpdateConstraint(float dt) = 0;
		};
	}
}