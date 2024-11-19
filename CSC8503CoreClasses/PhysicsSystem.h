#pragma once
#include "GameWorld.h"

namespace NCL {
	namespace CSC8503 {
		namespace Gravity {
			static const Vector3 Earth = Vector3(0.0f, -9.8f, 0.0f);
			static const Vector3 Mars = Vector3(0.0f, -3.7f, 0.0f);
			static const Vector3 Ceres = Vector3(0.0f, -0.3f, 0.0f);
			// Spin gravity goes away from the center of mass, a game would probably just invert the skybox...
			static const Vector3 SpinCeres = -Earth / 3.0f;
		};

		class PhysicsSystem	{
		public:
			PhysicsSystem(GameWorld& g);
			~PhysicsSystem();

			void Clear();

			void Update(float dt);

			void UseGravity(bool state) {
				applyGravity = state;
			}

			void SetGlobalDamping(float d) {
				globalDamping = d;
			}

			void SetGravity(const Vector3& g);
		protected:
			void BasicCollisionDetection();
			void BroadPhase();
			void NarrowPhase();

			void ClearForces();

			void IntegrateAccel(float dt);
			void integrateObjectAccel(PhysicsObject& object, float dt);
			void IntegrateVelocity(float dt);
			void integrateObjectVelocity(Transform& transform, PhysicsObject& object, float dt, float dampenFactor);

			void UpdateConstraints(float dt);

			void UpdateCollisionList();
			void UpdateObjectAABBs();

			void ImpulseResolveCollision(GameObject& a , GameObject&b, CollisionDetection::ContactPoint& p) const;

			GameWorld& gameWorld;

			bool	applyGravity;
			Vector3 gravity;
			float	dTOffset;
			float	globalDamping;

			std::set<CollisionDetection::CollisionInfo> allCollisions;
			std::set<CollisionDetection::CollisionInfo> broadphaseCollisions;
			std::vector<CollisionDetection::CollisionInfo> broadphaseCollisionsVec;
			bool useBroadPhase		= true;
			int numCollisionFrames	= 5;
		};
	}
}

