#pragma once
#include "NavigationMap.h"
#include <string>
namespace NCL {
	namespace CSC8503 {
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
			SearchNode* parent;
			GridNode* node;

			float currentCost;
			float currentPlusHeuristic;
		};

		class NavigationGrid : public NavigationMap	{
		public:
			NavigationGrid();
			NavigationGrid(const std::string&filename);
			~NavigationGrid();

			bool FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) const override;
				
		protected:
			bool		NodeInList(SearchNode* n, std::vector<SearchNode*>& list) const;
			bool NodeInList(GridNode* n, std::vector<SearchNode*>& list) const;
			SearchNode* FindNode(GridNode* n, std::vector<SearchNode*>& list) const;
			SearchNode*	RemoveBestNode(std::vector<SearchNode*>& list) const;
			float		Heuristic(GridNode* hNode, GridNode* endNode) const;
			int nodeSize;
			int gridWidth;
			int gridHeight;

			GridNode* allNodes;
		};
	}
}

