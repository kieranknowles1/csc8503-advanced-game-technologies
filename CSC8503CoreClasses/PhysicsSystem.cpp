#include "PhysicsSystem.h"

#include <cassert>

#include "PhysicsObject.h"
#include "GameObject.h"
#include "CollisionDetection.h"
#include "Quaternion.h"

#include "Constraint.h"

#include "Debug.h"
#include "Window.h"
#include <functional>

#include "RenderObject.h"

using namespace NCL;
using namespace CSC8503;

PhysicsSystem::PhysicsSystem(GameWorld& g)
	: gameWorld(g)
{
	dTOffset		= 0.0f;
	globalDamping	= 0.9f;
	SetGravity(Vector3(0.0f, -9.8f, 0.0f));
}

PhysicsSystem::~PhysicsSystem()	{
}

void PhysicsSystem::SetGravity(const Vector3& g) {
	gravity = g;
}

/*

If the 'game' is ever reset, the PhysicsSystem must be
'cleared' to remove any old collisions that might still
be hanging around in the collision list. If your engine
is expanded to allow objects to be removed from the world,
you'll need to iterate through this collisions list to remove
any collisions they are in.

*/
void PhysicsSystem::Clear() {
	allCollisions.clear();
}

/*

This is the core of the physics engine update

*/

bool useSimpleContainer = false;

int constraintIterationCount = 10;

//This is the fixed timestep we'd LIKE to have
const int   idealHZ = 120;
const float idealDT = 1.0f / idealHZ;

/*
This is the fixed update we actually have...
If physics takes too long it starts to kill the framerate, it'll drop the
iteration count down until the FPS stabilises, even if that ends up
being at a low rate.
*/
int realHZ		= idealHZ;
float realDT	= idealDT;

void PhysicsSystem::Update(float dt) {
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::B)) {
		useBroadPhase = !useBroadPhase;
		std::cout << "Setting broadphase to " << useBroadPhase << std::endl;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::N)) {
		useSimpleContainer = !useSimpleContainer;
		std::cout << "Setting broad container to " << useSimpleContainer << std::endl;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::I)) {
		constraintIterationCount--;
		std::cout << "Setting constraint iterations to " << constraintIterationCount << std::endl;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::O)) {
		constraintIterationCount++;
		std::cout << "Setting constraint iterations to " << constraintIterationCount << std::endl;
	}

	dTOffset += dt; //We accumulate time delta here - there might be remainders from previous frame!

	GameTimer t;
	t.GetTimeDeltaSeconds();

	if (useBroadPhase) {
		UpdateObjectAABBs();
	}
	int iteratorCount = 0;
	while(dTOffset > realDT) {
		IntegrateAccel(realDT); //Update accelerations from external forces
		if (useBroadPhase) {
			BroadPhase();
			NarrowPhase();
		}
		else {
			BasicCollisionDetection();
		}

		//This is our simple iterative solver -
		//we just run things multiple times, slowly moving things forward
		//and then rechecking that the constraints have been met
		float constraintDt = realDT /  (float)constraintIterationCount;
		for (int i = 0; i < constraintIterationCount; ++i) {
			UpdateConstraints(constraintDt);
		}
		IntegrateVelocity(realDT); //update positions from new velocity changes

		dTOffset -= realDT;
		iteratorCount++;
	}

	ClearForces();	//Once we've finished with the forces, reset them to zero

	UpdateCollisionList(); //Remove any old collisions and fire events

	t.Tick();
	float updateTime = t.GetTimeDeltaSeconds();

	//Uh oh, physics is taking too long...
	if (updateTime > realDT) {
		realHZ /= 2;
		realDT *= 2;
		std::cout << "Dropping iteration count due to long physics time...(now " << realHZ << ")\n";
	}
	else if(dt*2 < realDT) { //we have plenty of room to increase iteration count!
		int temp = realHZ;
		realHZ *= 2;
		realDT /= 2;

		if (realHZ > idealHZ) {
			realHZ = idealHZ;
			realDT = idealDT;
		}
		if (temp != realHZ) {
			std::cout << "Raising iteration count due to short physics time...(now " << realHZ << ")\n";
		}
	}
}

/*
Later on we're going to need to keep track of collisions
across multiple frames, so we store them in a set.

The first time they are added, we tell the objects they are colliding.
The frame they are to be removed, we tell them they're no longer colliding.

From this simple mechanism, we we build up gameplay interactions inside the
OnCollisionBegin / OnCollisionEnd functions (removing health when hit by a
rocket launcher, gaining a point when the player hits the gold coin, and so on).
*/
void PhysicsSystem::UpdateCollisionList() {
	for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
		if ((*i).framesLeft == numCollisionFrames) {
			i->a->OnCollisionBegin(i->b);
			i->b->OnCollisionBegin(i->a);
		}

		CollisionDetection::CollisionInfo& in = const_cast<CollisionDetection::CollisionInfo&>(*i);
		in.framesLeft--;

		if ((*i).framesLeft < 0) {
			i->a->OnCollisionEnd(i->b);
			i->b->OnCollisionEnd(i->a);
			i = allCollisions.erase(i);
		}
		else {
			++i;
		}
	}
}

void PhysicsSystem::UpdateObjectAABBs() {
	gameWorld.OperateOnContents(
		[](GameObject* g) {
			g->UpdateBroadphaseAABB();
		}
	);
}

/*

This is how we'll be doing collision detection in tutorial 4.
We step thorugh every pair of objects once (the inner for loop offset
ensures this), and determine whether they collide, and if so, add them
to the collision set for later processing. The set will guarantee that
a particular pair will only be added once, so objects colliding for
multiple frames won't flood the set with duplicates.
*/
void PhysicsSystem::BasicCollisionDetection() {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	gameWorld.GetObjectIterators(first, last);

	for (auto i = first; i != last; i++) {
		if ((*i)->GetPhysicsObject() == nullptr) {
			continue;
		}
		// Only check for collisions with objects that come after the current one,
		// this ensures each pair is checked exactly once
		for (auto j = i + 1; j != last; j++) {
			if ((*j)->GetPhysicsObject() == nullptr) {
				continue;
			}
			// TODO: Check masks
			CollisionDetection::CollisionInfo info;
			if (CollisionDetection::ObjectIntersection(*i, *j, info)) {
				ResolveCollision(*info.a, *info.b, info.point); // Resolve the collision (separate the objects
				info.framesLeft = numCollisionFrames;
				allCollisions.insert(info);
			}
		}
	}
}

void PhysicsSystem::ResolveCollision(GameObject& a, GameObject& b, CollisionDetection::ContactPoint& p) const {
	if (a.IsTrigger() || b.IsTrigger()) {
		// Don't resolve triggers
		// UpdateCollisionList is responsible for firing events
		return;
	}

	switch (resolutionType) {
	case CollisionResolution::Impulse:
		return ImpulseResolveCollision(a, b, p);
	default: assert(false); // Not implemented
	}
}

/*

In tutorial 5, we start determining the correct response to a collision,
so that objects separate back out.

*/
void PhysicsSystem::ImpulseResolveCollision(GameObject& a, GameObject& b, CollisionDetection::ContactPoint& p) const {
	PhysicsObject* physA = a.GetPhysicsObject();
	PhysicsObject* physB = b.GetPhysicsObject();

	// These should have been filtered out by the broadphase
	assert(physA && physB && "ImpulseResolveCollision objects have no physics object!");

	Transform& transformA = a.GetTransform();
	Transform& transformB = b.GetTransform();
	float totalMass = physA->GetInverseMass() + physB->GetInverseMass();
	if (totalMass == 0) {
		return; // Two static objects, can't have any impulse
	}

	// Separate using projection
	transformA.SetPosition(transformA.GetPosition() - (p.normal * p.penetration * (physA->GetInverseMass() / totalMass)));
	transformB.SetPosition(transformB.GetPosition() + (p.normal * p.penetration * (physB->GetInverseMass() / totalMass)));

	// Apply impulse
	// See `Collision Response/Combined Impulse Calculation` in notes
	Vector3 angularVelocityA = Vector::Cross(physA->GetAngularVelocity(), p.localA);
	Vector3 angularVelocityB = Vector::Cross(physB->GetAngularVelocity(), p.localB);

	Vector3 fullVelocityA = physA->GetLinearVelocity() + angularVelocityA;
	Vector3 fullVelocityB = physB->GetLinearVelocity() + angularVelocityB;

	Vector3 contactVelocity = fullVelocityB - fullVelocityA;

	float impulseForce = Vector::Dot(contactVelocity, p.normal);
	// Calculate inertia
	Vector3 inertiaA = Vector::Cross(physA->GetInertiaTensor() * Vector::Cross(p.localA, p.normal), p.localA);
	Vector3 inertiaB = Vector::Cross(physB->GetInertiaTensor() * Vector::Cross(p.localB, p.normal), p.localB);
	float angularEffect = Vector::Dot(inertiaA + inertiaB, p.normal);

	float restitution = physA->GetElasticity() * physB->GetElasticity();
	// Divisor of the impulse calculation
	float j = ( -(1.0f + restitution) * impulseForce) / (totalMass - angularEffect);
	Vector3 fullImpulse = p.normal * j;

	// Apply the impulses in opposite directions
	physA->ApplyLinearImpulse(-fullImpulse);
	physB->ApplyLinearImpulse(fullImpulse);

	physA->ApplyAngularImpulse(Vector::Cross(p.localA, -fullImpulse));
	physB->ApplyAngularImpulse(Vector::Cross(p.localB, fullImpulse));
}

void PhysicsSystem::rebuildStaticsTree() {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	gameWorld.GetObjectIterators(first, last);

	staticsTree = std::make_unique<QuadTree<GameObject*>>(Vector2(1024, 1024), 7, 6);
	for (auto i = first; i != last; i++) {
		GameObject* object = *i;
		if (object->getPhysicsType() != GameObject::PhysicsType::Static) {
			continue;
		}
		Vector3 halfSizes;
		object->GetBroadphaseAABB(halfSizes);
		Vector3 pos = object->GetTransform().GetPosition();
		staticsTree->Insert(object, pos, halfSizes);
	}
}

/*

Later, we replace the BasicCollisionDetection method with a broadphase
and a narrowphase collision detection method. In the broad phase, we
split the world up using an acceleration structure, so that we can only
compare the collisions that we absolutely need to.

*/
void PhysicsSystem::BroadPhase() {
	broadphaseCollisions.clear();
	if (staticsTree == nullptr) {
		rebuildStaticsTree();
	}
	QuadTree<GameObject*> tree(Vector2(1024, 1024), 7, 6);

	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	gameWorld.GetObjectIterators(first, last);
	for (auto i = first; i != last; i++) {
		GameObject* object = *i;
		if (object->getPhysicsType() != GameObject::PhysicsType::Dynamic) {
			continue;
		}

		Vector3 halfSizes;
		object->GetBroadphaseAABB(halfSizes);
		Vector3 pos = object->GetTransform().GetPosition();
		tree.Insert(object, pos, halfSizes);
	}

	// Dynamic objects can collide with statics in the same leaf
	tree.OperateOnContents(
		[&](std::list<QuadTreeEntry<GameObject*>>& data) {
			CollisionDetection::CollisionInfo info;
			for (auto i = data.begin(); i != data.end(); i++) {
				// Each pair of objects is only added once
				for (auto j = std::next(i); j != data.end(); j++) {
					info.a = std::min((*i).object, (*j).object);
					info.b = std::max((*i).object, (*j).object);
					broadphaseCollisions.insert(info);
				}

				// Check for collisions with static objects
				auto& staticsLeaf = staticsTree->GetContainingNode((*i).pos, (*i).size);
				QuadTreeNode<GameObject*>::QuadTreeFunc func = [&](std::list<QuadTreeEntry<GameObject*>>& staticData) {
					for (auto j = staticData.begin(); j != staticData.end(); j++) {
						info.a = (*i).object;
						info.b = (*j).object;
						broadphaseCollisions.insert(info);
					}
				};
				staticsLeaf.OperateOnContents(func);
			}
		});
}

/*

The broadphase will now only give us likely collisions, so we can now go through them,
and work out if they are truly colliding, and if so, add them into the main collision list
*/
void PhysicsSystem::NarrowPhase() {
	for (auto i = broadphaseCollisions.begin(); i != broadphaseCollisions.end(); i++) {
		CollisionDetection::CollisionInfo info = *i;
		if (CollisionDetection::ObjectIntersection(info.a, info.b, info)) {
			info.framesLeft = numCollisionFrames;
			ResolveCollision(*info.a, *info.b, info.point);
			allCollisions.insert(info);
		}
	}
}

/*
Integration of acceleration and velocity is split up, so that we can
move objects multiple times during the course of a PhysicsUpdate,
without worrying about repeated forces accumulating etc.

This function will update both linear and angular acceleration,
based on any forces that have been accumulated in the objects during
the course of the previous game frame.
*/
void PhysicsSystem::IntegrateAccel(float dt) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	gameWorld.GetObjectIterators(first, last);
	for (auto i = first; i != last; i++) {
		PhysicsObject* object = (*i)->GetPhysicsObject();
		if (object != nullptr) {
			integrateObjectAccel(*object, dt);
		}
	}
}

void PhysicsSystem::integrateObjectAccel(PhysicsObject& object, float dt) {
	float inverseMass = object.GetInverseMass();
	Vector3 linearVelocity = object.GetLinearVelocity();
	Vector3 force = object.GetForce();
	// Newton's second law, F=ma, therefore a=Fm^-1
	Vector3 acceleration = force * inverseMass;

	// Gravity is a constant acceleration, unless the object has infinite mass
	if (inverseMass > 0) {
		acceleration += gravity;
	}
	// Integrate linear acceleration using implicit Euler
	linearVelocity += acceleration * dt;
	object.SetLinearVelocity(linearVelocity);

	// Apply angular acceleration
	Vector3 torque = object.GetTorque();
	Vector3 angularVelocity = object.GetAngularVelocity();

	object.UpdateInertiaTensor(); // Rotate the inertia tensor to the object's local space
	Vector3 angularAcceleration = object.GetInertiaTensor() * torque;

	angularVelocity += angularAcceleration * dt;
	object.SetAngularVelocity(angularVelocity);
}

/*
This function integrates linear and angular velocity into
position and orientation. It may be called multiple times
throughout a physics update, to slowly move the objects through
the world, looking for collisions.
*/
void PhysicsSystem::IntegrateVelocity(float dt) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	gameWorld.GetObjectIterators(first, last);
	// Global dampening represents the fraction of velocity that remains after 1 second
	// Taking globalDamping^dt gives the fraction that remains after dt seconds, which converges to 0 at dt=inf and 1 at dt=0
	// https://www.desmos.com/calculator/a3sz0jbhpo
	// Using 1-(d*x) was a poor choice, as it doesn't converge to 0, is heavily dependent on framerate, and if dt > (1/d), the object will move backwards
	float dampening = pow(globalDamping, dt);

	for (auto i = first; i != last; i++) {
		PhysicsObject* object = (*i)->GetPhysicsObject();
		if (object != nullptr) {
			integrateObjectVelocity((*i)->GetTransform(), *object, dt, dampening);
		}
	}
}

void PhysicsSystem::integrateObjectVelocity(Transform& transform, PhysicsObject& object, float dt, float dampenFactor) {
	float linearDamping = dampenFactor * std::pow(object.GetLinearDamping(), dt);
	float angularDamping = dampenFactor * std::pow(object.GetAngularDamping(), dt);

	Vector3 position = transform.GetPosition();
	Vector3 linearVelocity = object.GetLinearVelocity();
	position += linearVelocity * dt;
	transform.SetPosition(position);

	// Dampen linear velocity
	linearVelocity = linearVelocity * linearDamping;
	object.SetLinearVelocity(linearVelocity);

	Quaternion orientation = transform.GetOrientation();
	Vector3 angularVelocity = object.GetAngularVelocity();

	// Magic *0.5f due to how quaternions work. Leave it to mathemagicians to figure out why
	// Then they can go outside and see the sun for the first time in years
	orientation = orientation + (Quaternion(angularVelocity * dt * 0.5f, 0.0f) * orientation);
	orientation.Normalise();
	transform.SetOrientation(orientation);

	// Dampen angular velocity
	angularVelocity = angularVelocity * angularDamping;
	object.SetAngularVelocity(angularVelocity);
}

/*
Once we're finished with a physics update, we have to
clear out any accumulated forces, ready to receive new
ones in the next 'game' frame.
*/
void PhysicsSystem::ClearForces() {
	gameWorld.OperateOnContents(
		[](GameObject* o) {
			o->GetPhysicsObject()->ClearForces();
		}
	);
}


/*

As part of the final physics tutorials, we add in the ability
to constrain objects based on some extra calculation, allowing
us to model springs and ropes etc.

*/
void PhysicsSystem::UpdateConstraints(float dt) {
	std::vector<Constraint*>::const_iterator first;
	std::vector<Constraint*>::const_iterator last;
	gameWorld.GetConstraintIterators(first, last);

	for (auto i = first; i != last; ++i) {
		(*i)->UpdateConstraint(dt);
	}
}
