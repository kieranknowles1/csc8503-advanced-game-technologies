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
			enum class CollisionResolution {
				Impulse,
			};

			PhysicsSystem(GameWorld& g);
			~PhysicsSystem();

			void Clear();

			void Update(float dt);

			void SetGlobalDamping(float d) {
				globalDamping = d;
			}

			void SetGravity(const Vector3& g);

			// Discard the statics tree and rebuild it next frame
			// Use when adding/removing/moving static objects
			// If an object will move frequently, it should be dynamic
			void dirtyStaticsTree() {
				staticsTree.reset();
			}

			void removeObject(GameObject* object);
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

			void ResolveCollision(GameObject& a, GameObject& b, CollisionDetection::ContactPoint& p) const;
			void ImpulseResolveCollision(GameObject& a , GameObject&b, CollisionDetection::ContactPoint& p) const;

			GameWorld& gameWorld;

			Vector3 gravity;
			float	dTOffset;
			float	globalDamping;

			CollisionResolution resolutionType = CollisionResolution::Impulse;

			void rebuildStaticsTree();
			std::unique_ptr<QuadTree<GameObject*>> staticsTree;

			std::set<CollisionDetection::CollisionInfo> allCollisions;
			std::set<CollisionDetection::CollisionInfo> broadphaseCollisions;
			std::vector<CollisionDetection::CollisionInfo> broadphaseCollisionsVec;
			bool useBroadPhase		= true;
			int numCollisionFrames	= 5;
		};
	}
}

