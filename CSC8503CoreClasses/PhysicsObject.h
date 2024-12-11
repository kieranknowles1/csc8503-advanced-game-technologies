#pragma once
using namespace NCL::Maths;

namespace NCL {
	class CollisionVolume;

	namespace CSC8503 {
		class Transform;

		class PhysicsObject	{
		public:
			PhysicsObject(Transform* parentTransform, const CollisionVolume* parentVolume);
			~PhysicsObject();

			Vector3 GetLinearVelocity() const {
				return linearVelocity;
			}

			Vector3 GetAngularVelocity() const {
				return angularVelocity;
			}

			Vector3 GetTorque() const {
				return torque;
			}

			Vector3 GetForce() const {
				return force;
			}

			void SetInverseMass(float invMass) {
				inverseMass = invMass;
			}

			float GetInverseMass() const {
				return inverseMass;
			}

			void ApplyAngularImpulse(const Vector3& force);
			void ApplyLinearImpulse(const Vector3& force);

			void AddForce(const Vector3& force);

			void AddForceAtPosition(const Vector3& force, const Vector3& position);

			void AddTorque(const Vector3& torque);


			void ClearForces();

			void SetLinearVelocity(const Vector3& v) {
				linearVelocity = v;
			}

			void SetAngularVelocity(const Vector3& v) {
				angularVelocity = v;
			}

			void InitCubeInertia();
			void InitSphereInertia();

			void UpdateInertiaTensor();

			Matrix3 GetInertiaTensor() const {
				return inverseInteriaTensor;
			}

			float GetElasticity() const {
				return elasticity;
			}
			void SetElasticity(float e) {
				elasticity = e;
			}

			float GetAngularDamping() const {
				return angularDamping;
			}
			void SetAngularDamping(float d) {
				angularDamping = d;
			}

			float GetLinearDamping() const {
				return linearDamping;
			}
			void SetLinearDamping(float d) {
				linearDamping = d;
			}

			// Apply an impulse to push an object towards a target velocity
			// Ignores the y axis
			// Force should be scaled by dt
			void pushTowardsVelocity(Vector3 targetVelocity, float force);

		protected:
			const CollisionVolume* volume;
			Transform*		transform;

			float inverseMass;
			float elasticity;
			float friction;

			// Damping values, multiplied by the global damping values
			float angularDamping = 1.0f;
			float linearDamping = 1.0f;

			//linear stuff
			Vector3 linearVelocity;
			Vector3 force;

			//angular stuff
			Vector3 angularVelocity;
			Vector3 torque;
			Vector3 inverseInertia;
			Matrix3 inverseInteriaTensor;
		};
	}
}

