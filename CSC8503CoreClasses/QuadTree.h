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

			void Insert(T& object, const Vector3& objectPos, const Vector3& objectSize, int depthLeft, int maxSize) {
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

			void DebugDraw() {
				if (children) {
					for (int i = 0; i < 4; i++) {
						children[i].DebugDraw();
					}
				}
				else {
					// TODO: Debug::DrawBox?
					// Bottom
					Debug::DrawLine(Vector3(position.x, 0, position.y), Vector3(position.x + size.x, 0, position.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x + size.x, 0, position.y), Vector3(position.x + size.x, 0, position.y + size.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x + size.x, 0, position.y + size.y), Vector3(position.x, 0, position.y + size.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x, 0, position.y + size.y), Vector3(position.x, 0, position.y), Vector4(1, 0, 0, 1));

					// Top
					Debug::DrawLine(Vector3(position.x, 1000, position.y), Vector3(position.x + size.x, 1000, position.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x + size.x, 1000, position.y), Vector3(position.x + size.x, 1000, position.y + size.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x + size.x, 1000, position.y + size.y), Vector3(position.x, 1000, position.y + size.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x, 1000, position.y + size.y), Vector3(position.x, 1000, position.y), Vector4(1, 0, 0, 1));

					// Sides
					Debug::DrawLine(Vector3(position.x, 0, position.y), Vector3(position.x, 1000, position.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x + size.x, 0, position.y), Vector3(position.x + size.x, 1000, position.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x + size.x, 0, position.y + size.y), Vector3(position.x + size.x, 1000, position.y + size.y), Vector4(1, 0, 0, 1));
					Debug::DrawLine(Vector3(position.x, 0, position.y + size.y), Vector3(position.x, 1000, position.y + size.y), Vector4(1, 0, 0, 1));
				}
			}

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

			void DebugDraw() {
				root.DebugDraw();
			}

			void OperateOnContents(typename QuadTreeNode<T>::QuadTreeFunc  func) {
				root.OperateOnContents(func);
			}

		protected:
			QuadTreeNode<T> root;
			int maxDepth;
			int maxSize;
		};
	}
}
