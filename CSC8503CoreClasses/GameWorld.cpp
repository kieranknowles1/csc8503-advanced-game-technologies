#include "GameWorld.h"
#include "GameObject.h"
#include "Constraint.h"
#include "CollisionDetection.h"
#include "Camera.h"


using namespace NCL;
using namespace NCL::CSC8503;

GameWorld::GameWorld()	{
	shuffleConstraints	= false;
	shuffleObjects		= false;
	worldIDCounter		= 0;
	worldStateCounter	= 0;
}

GameWorld::~GameWorld()	{
}

void GameWorld::Clear() {
	gameObjects.clear();
	constraints.clear();
	worldIDCounter		= 0;
	worldStateCounter	= 0;
	taggedObjects.clear();
}

void GameWorld::ClearAndErase() {
	for (auto& i : gameObjects) {
		delete i;
	}
	for (auto& i : constraints) {
		delete i;
	}
	Clear();
}

void GameWorld::AddGameObject(GameObject* o) {
	gameObjects.emplace_back(o);
	taggedObjects.insert(std::make_pair(o->getTag(), o));
	o->SetWorld(this);
	o->SetWorldID(worldIDCounter++);
	worldStateCounter++;
}

void GameWorld::RemoveGameObject(GameObject* o, bool andDelete) {
	gameObjects.erase(std::find(gameObjects.begin(), gameObjects.end(), o));
	auto range = taggedObjects.equal_range(o->getTag());
	for (auto i = range.first; i != range.second; ++i) {
		if (i->second == o) {
			taggedObjects.erase(i);
			break;
		}
	}
	o->SetWorld(nullptr);
	if (andDelete) {
		delete o;
	}
	worldStateCounter++;
}

void GameWorld::GetObjectIterators(
	GameObjectIterator& first,
	GameObjectIterator& last) const {

	first	= gameObjects.begin();
	last	= gameObjects.end();
}

void GameWorld::UpdateWorld(float dt) {
	auto rng = std::default_random_engine{};

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine e(seed);

	if (shuffleObjects) {
		std::shuffle(gameObjects.begin(), gameObjects.end(), e);
	}

	if (shuffleConstraints) {
		std::shuffle(constraints.begin(), constraints.end(), e);
	}

	for (auto& i : gameObjects) {
		i->OnUpdate(dt);
	}
}

bool GameWorld::Raycast(Ray& r, RayCollision& closestCollision, bool closestObject, GameObject* ignoreThis) const {
	//The simplest raycast just goes through each object and sees if there's a collision
	RayCollision collision;


	for (auto& i : gameObjects) {
		if (!i->GetBoundingVolume()) { //objects might not be collideable etc...
			continue;
		}
		if (i == ignoreThis) {
			continue;
		}
		// Skip objects that are masked out
		if (!r.getMask().matches(i->getLayer())) {
			continue;
		}
		RayCollision thisCollision;
		if (CollisionDetection::RayIntersection(r, *i, thisCollision)) {

			if (!closestObject) {
				closestCollision		= collision;
				closestCollision.node = i;
				return true;
			}
			else {
				if (thisCollision.rayDistance < collision.rayDistance) {
					thisCollision.node = i;
					collision = thisCollision;
				}
			}
		}
	}
	if (collision.node) {
		closestCollision		= collision;
		closestCollision.node	= collision.node;
		return true;
	}
	return false;
}

bool GameWorld::hasLineOfSight(GameObject* from, GameObject* to, float maxDistance) const
{
	// Ray casts are fairly expensive. If we're too far then it's impossible
	// to have line of sight
	float distance = Vector::Length(to->GetTransform().GetPosition() - from->GetTransform().GetPosition());
	if (distance > maxDistance) {
		return false;
	}

	Vector3 direction = Vector::Normalise(
		to->GetTransform().GetPosition() - from->GetTransform().GetPosition()
	);
	Ray ray(from->GetTransform().GetPosition(), direction);
	RayCollision closest;
	bool hit = Raycast(ray, closest, true, from);
	return hit && closest.node == to;
}


/*
Constraint Tutorial Stuff
*/

void GameWorld::AddConstraint(Constraint* c) {
	constraints.emplace_back(c);
}

void GameWorld::RemoveConstraint(Constraint* c, bool andDelete) {
	constraints.erase(std::remove(constraints.begin(), constraints.end(), c), constraints.end());
	if (andDelete) {
		delete c;
	}
}

void GameWorld::GetConstraintIterators(
	std::vector<Constraint*>::const_iterator& first,
	std::vector<Constraint*>::const_iterator& last) const {
	first	= constraints.begin();
	last	= constraints.end();
}
