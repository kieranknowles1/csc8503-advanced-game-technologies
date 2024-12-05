#pragma once
#include <random>
#include <vector>
#include <map>
#include <string>

#include "Ray.h"
#include "CollisionDetection.h"
#include "QuadTree.h"
namespace NCL {
		// Declare RNG here as an alias, so that changing it would be easy
		// mt19937 is a common standard, but quite large to transfer over the network
		using Rng = std::mt19937;

		class Camera;
		using Maths::Ray;
	namespace CSC8503 {
		class GameObject;
		enum class GameObject::Tag;
		class Constraint;

		typedef std::function<void(GameObject*)> GameObjectFunc;
		typedef std::vector<GameObject*>::const_iterator GameObjectIterator;
		using TaggedObjects = std::multimap<GameObject::Tag, GameObject*>;

		class GameWorld	{
		public:
			GameWorld();
			~GameWorld();

			void Clear();
			void ClearAndErase();

			void AddGameObject(GameObject* o);
			void RemoveGameObject(GameObject* o, bool andDelete = false);

			void AddConstraint(Constraint* c);
			void RemoveConstraint(Constraint* c, bool andDelete = false);

			PerspectiveCamera& GetMainCamera()  {
				return mainCamera;
			}

			void ShuffleConstraints(bool state) {
				shuffleConstraints = state;
			}

			void ShuffleObjects(bool state) {
				shuffleObjects = state;
			}

			bool Raycast(Ray& r, RayCollision& closestCollision, bool closestObject = false, GameObject* ignore = nullptr) const;

			// Is there an unobstructed line of sight between two objects?
			bool hasLineOfSight(GameObject* from, GameObject* to) const;

			virtual void UpdateWorld(float dt);

			void OperateOnContents(GameObjectFunc f);

			GameObject* getObject(int id) const {
				for (auto& i : gameObjects) {
					if (i->GetWorldID() == id) {
						return i;
					}
				}
				return nullptr;
			}

			void GetObjectIterators(
				GameObjectIterator& first,
				GameObjectIterator& last) const;

			void GetConstraintIterators(
				std::vector<Constraint*>::const_iterator& first,
				std::vector<Constraint*>::const_iterator& last) const;

			int GetWorldStateID() const {
				return worldStateCounter;
			}

			void getTaggedObjects(GameObject::Tag tag, TaggedObjects::iterator& outBegin, TaggedObjects::iterator& outEnd) {
				auto range = taggedObjects.equal_range(tag);
				outBegin = range.first;
				outEnd = range.second;
			}
		protected:
			std::vector<GameObject*> gameObjects;
			std::vector<Constraint*> constraints;

			TaggedObjects taggedObjects;

			PerspectiveCamera mainCamera;

			bool shuffleConstraints;
			bool shuffleObjects;
			int		worldIDCounter;
			int		worldStateCounter;
		};
	}
}

