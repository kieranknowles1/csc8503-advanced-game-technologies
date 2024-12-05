#pragma once
#include "Transform.h"
#include "CollisionVolume.h"
#include "LayerMask.h"
#include "PhysicsObject.h"

using std::vector;

namespace NCL::CSC8503 {
	class NetworkObject;
	class RenderObject;
	class PhysicsObject;
	class GameWorld;

	class GameObject	{
	public:
		enum class Tag {
			None,
			Player,
		};

		enum class PhysicsType {
			// No physics object
			None,
			// Infinite mass, immovable
			Static,
			// Finite mass, movable
			Dynamic,
		};

		GameObject(const std::string& name = "");
		~GameObject();

		void SetBoundingVolume(CollisionVolume* vol) {
			boundingVolume = vol;
		}

		const CollisionVolume* GetBoundingVolume() const {
			return boundingVolume;
		}

		bool IsActive() const {
			return isActive;
		}

		PhysicsType getPhysicsType() const {
			if (physicsObject == nullptr || boundingVolume == nullptr) {
				return PhysicsType::None;
			}
			return physicsObject->GetInverseMass() == 0.0f ? PhysicsType::Static : PhysicsType::Dynamic;
		}

		Transform& GetTransform() {
			return transform;
		}

		RenderObject* GetRenderObject() const {
			return renderObject;
		}

		PhysicsObject* GetPhysicsObject() const {
			return physicsObject;
		}

		NetworkObject* GetNetworkObject() const {
			return networkObject;
		}

		void SetRenderObject(RenderObject* newObject) {
			renderObject = newObject;
		}

		void SetPhysicsObject(PhysicsObject* newObject) {
			physicsObject = newObject;
		}

		void SetNetworkObject(NetworkObject* newObject) {
			networkObject = newObject;
		}

		const std::string& GetName() const {
			return name;
		}

		virtual void OnCollisionBegin(GameObject* otherObject) {
			//std::cout << "OnCollisionBegin event occured!\n";
		}

		virtual void OnCollisionEnd(GameObject* otherObject) {
			//std::cout << "OnCollisionEnd event occured!\n";
		}

		virtual void OnUpdate(float dt) {
			//std::cout << "OnUpdate event occured!\n";
		}

		// Get the overall type of this object. Should return the same value for the object's entire lifetime
		virtual Tag getTag() const {
			return Tag::None;
		}

		bool GetBroadphaseAABB(Vector3&outsize) const;

		void UpdateBroadphaseAABB();

		void SetWorldID(int newID) {
			worldID = newID;
		}

		int		GetWorldID() const {
			return worldID;
		}

		void SetWorld(GameWorld* world) {
			this->world = world;
		}
		GameWorld* GetWorld() const {
			return world;
		}

		void setLayer(LayerMask::Index layer) {
			this->layer = layer;
		}
		LayerMask::Index getLayer() const {
			return layer;
		}

		bool IsTrigger() const {
			return trigger;
		}
		void SetTrigger(bool state) {
			trigger = state;
		}

		void SetDefaultTransform(const Transform& t) {
			defaultTransform = t;
		}

		virtual void Reset() {
			transform = defaultTransform;
			if (physicsObject) {
				physicsObject->SetLinearVelocity(Vector3());
				physicsObject->SetAngularVelocity(Vector3());
			}
		}
	protected:
		Transform transform;
		Transform defaultTransform;

		GameWorld* world;

		CollisionVolume* boundingVolume;
		PhysicsObject* physicsObject;
		RenderObject* renderObject;
		NetworkObject* networkObject;

		// If true, this object will not respond to collisions but still cause/recieve events
		bool trigger = false;
		LayerMask::Index layer = LayerMask::Index::Default;

		bool isActive;
		int worldID;
		std::string	name;

		Vector3 broadphaseAABB;
	};
}

