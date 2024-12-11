#pragma once
#include <cassert>

#include "CollisionDetection.h"
#include "Debug.h"

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<class T>
		class QuadTree;

		template<class T>
		struct QuadTreeEntry {
			Vector3 pos;
			Vector3 size;
			T object;

			QuadTreeEntry(T obj, Vector3 pos, Vector3 size) {
				object		= obj;
				this->pos	= pos;
				this->size	= size;
			}
		};

		template<class T>
		class QuadTreeNode	{
		public:
			typedef std::function<void(std::list<QuadTreeEntry<T>>&)> QuadTreeFunc;

			void OperateOnContents(QuadTreeFunc& func) {
				if (children) {
					for (int i = 0; i < 4; i++) {
						children[i].OperateOnContents(func);
					}
				}
				// Only leaf nodes track their contents
				else if (!contents.empty()) {
					func(contents);
				}
			}
		protected:
			friend class QuadTree<T>;

			QuadTreeNode() {}

			QuadTreeNode(Vector2 pos, Vector2 size) {
				children		= nullptr;
				this->position	= pos;
				this->size		= size;
			}

			~QuadTreeNode() {
				delete[] children;
			}

			// Get the smallest node that completely contains the given AABB
			QuadTreeNode* GetContainingNode(const Vector3& objectPos, const Vector3& objectSize) {
				// Shrink our size by the object size to filter out partial overlaps
				Vector3 shrunkSize = Vector3(size.x - objectSize.x, 1000.0f, size.y - objectSize.y);

				bool thisFits = CollisionDetection::AABBTest(objectPos, Vector3(position.x, 0, position.y), objectSize, shrunkSize);
				if (!thisFits) {
					// If we don't fit, then no children can fit either
					return nullptr;
				}

				// See if it fits in any children
				if (children) {
					for (int i = 0; i < 4; i++) {
						QuadTreeNode* node = children[i].GetContainingNode(objectPos, objectSize);
						if (node) {
							return node;
						}
					}
				}

				// If no children fit, then we are the smallest node that fits
				return this;
			}

			// Insert an object into all leaves that it touches
			void Insert(T& object, const Vector3& objectPos, const Vector3& objectSize, int depthLeft, int maxSize) {
				// We want to check for any partial overlaps, so don't shrink the size
				if (!CollisionDetection::AABBTest(objectPos, Vector3(position.x, 0, position.y), objectSize, Vector3(size.x, 1000.0f, size.y))) {
					return; // Not in this quad
				}
				if (children) { // Not a leaf, place in our children
					for (int i = 0; i < 4; i++) {
						children[i].Insert(object, objectPos, objectSize, depthLeft - 1, maxSize);
					}
				}
				else { // A leaf node, expand
					contents.push_back(QuadTreeEntry<T>(object, objectPos, objectSize));
					if (contents.size() > maxSize && depthLeft > 0) {
						// Shouldn't be possible, unless something went badly wrong!
						assert(children == nullptr && "Inserting into a leaf with contents!");
						Split();
						// Reinsert contents into children
						for (auto& i : contents) {
							for (int j = 0; j < 4; j++) {
								auto entry = i;
								children[j].Insert(entry.object, entry.pos, entry.size, depthLeft - 1, maxSize);
							}
						}
						contents.clear(); // No longer a leaf!
					}
				}
			}

			void Split() {
				Vector2 halfSize = size / 2.0f;
				children = new QuadTreeNode<T>[4];
				children[0] = QuadTreeNode<T>(position + Vector2(-halfSize.x, halfSize.y), halfSize);
				children[1] = QuadTreeNode<T>(position + Vector2(halfSize.x, halfSize.y), halfSize);
				children[2] = QuadTreeNode<T>(position + Vector2(-halfSize.x, -halfSize.y), halfSize);
				children[3] = QuadTreeNode<T>(position + Vector2(halfSize.x, -halfSize.y), halfSize);
			}

			void DebugDraw(Vector4 color) {
				if (children) {
					for (int i = 0; i < 4; i++) {
						children[i].DebugDraw(color);
					}
				}
				else {
					Debug::DrawAABB(Vector3(position.x, 500, position.y), Vector3(size.x, 500, size.y), color);
				}
			}

		protected:
			std::list< QuadTreeEntry<T> >	contents;

			Vector2 position;
			Vector2 size;

			QuadTreeNode<T>* children;
		};
	}
}


namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<class T>
		class QuadTree
		{
		public:
			QuadTree(Vector2 size, int maxDepth = 6, int maxSize = 5){
				root = QuadTreeNode<T>(Vector2(), size);
				this->maxDepth	= maxDepth;
				this->maxSize	= maxSize;
			}
			~QuadTree() {
			}

			void Insert(T object, const Vector3& pos, const Vector3& size) {
				root.Insert(object, pos, size, maxDepth, maxSize);
			}

			void DebugDraw(Vector4 color) {
				root.DebugDraw(color);
			}

			void OperateOnContents(typename QuadTreeNode<T>::QuadTreeFunc func) {
				root.OperateOnContents(func);
			}

			// Get the smallest node that completely contains the given AABB
			QuadTreeNode<T>& GetContainingNode(const Vector3& pos, const Vector3& size) {
				return *root.GetContainingNode(pos, size);
			}

		protected:
			QuadTreeNode<T> root;
			int maxDepth;
			int maxSize;
		};
	}
}
