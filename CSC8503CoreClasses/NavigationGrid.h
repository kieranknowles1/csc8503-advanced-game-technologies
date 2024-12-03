#pragma once
#include "NavigationMap.h"
#include <string>
namespace NCL {
	namespace CSC8503 {
		const constexpr char WALL_NODE = 'x';
		const constexpr char FLOOR_NODE = '.';

		struct GridNode {
			GridNode* connected[4];
			int		  costs[4];

			Vector3		position;

			int type;

			GridNode() {
				for (int i = 0; i < 4; ++i) {
					connected[i] = nullptr;
					costs[i] = 0;
				}
				type = 0;
			}
			~GridNode() {	}
		};

		struct SearchNode {
			static SearchNode empty() {
				return SearchNode(nullptr, nullptr, 0, 0);
			}
			bool isEmpty() const {
				return node == nullptr;
			}

			GridNode* parent;
			GridNode* node;

			// g(n), cost from start to n
			float currentCost;
			// g(n) + h(n), cost from start to n + heuristic cost from n to goal
			float currentPlusHeuristic;

			// Comparator used in priority_queue<T>
			float operator>(const SearchNode& other) const {
				return currentPlusHeuristic > other.currentPlusHeuristic;
			}
		};

		class NavigationGrid : public NavigationMap	{
		public:
			NavigationGrid();
			NavigationGrid(const std::string&filename, Vector3 offset = Vector3());
			~NavigationGrid();

			int getNodeCount() const {
				return gridWidth * gridHeight;
			}
			int getNodeSize() const {
				return nodeSize;
			}
			GridNode* getNode(int idx) const {
				return &allNodes[idx];
			}

			bool FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) const override;

			// Draw the grid for debugging
			// White: Connection between nodes
			// Red: Wall
			void debugDraw();
		protected:
			float		Heuristic(GridNode* hNode, GridNode* endNode) const;
			Vector3 offset;
			int nodeSize;
			int gridWidth;
			int gridHeight;

			GridNode* allNodes;
		};
	}
}

