#include "NavigationGrid.h"
#include "Assets.h"

#include <fstream>

using namespace NCL;
using namespace CSC8503;

const int LEFT_NODE		= 0;
const int RIGHT_NODE	= 1;
const int TOP_NODE		= 2;
const int BOTTOM_NODE	= 3;

const char WALL_NODE	= 'x';
const char FLOOR_NODE	= '.';

NavigationGrid::NavigationGrid()	{
	nodeSize	= 0;
	gridWidth	= 0;
	gridHeight	= 0;
	allNodes	= nullptr;
}

NavigationGrid::NavigationGrid(const std::string&filename) : NavigationGrid() {
	std::ifstream infile(Assets::DATADIR + filename);

	infile >> nodeSize;
	infile >> gridWidth;
	infile >> gridHeight;

	allNodes = new GridNode[gridWidth * gridHeight];

	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode&n = allNodes[(gridWidth * y) + x];
			char type = 0;
			infile >> type;
			n.type = type;
			n.position = Vector3((float)(x * nodeSize), 0, (float)(y * nodeSize));
		}
	}
	
	//now to build the connectivity between the nodes
	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode&n = allNodes[(gridWidth * y) + x];		

			if (y > 0) { //get the above node
				n.connected[0] = &allNodes[(gridWidth * (y - 1)) + x];
			}
			if (y < gridHeight - 1) { //get the below node
				n.connected[1] = &allNodes[(gridWidth * (y + 1)) + x];
			}
			if (x > 0) { //get left node
				n.connected[2] = &allNodes[(gridWidth * (y)) + (x - 1)];
			}
			if (x < gridWidth - 1) { //get right node
				n.connected[3] = &allNodes[(gridWidth * (y)) + (x + 1)];
			}
			for (int i = 0; i < 4; ++i) {
				if (n.connected[i]) {
					if (n.connected[i]->type == '.') {
						n.costs[i]		= 1;
					}
					if (n.connected[i]->type == 'x') {
						n.connected[i] = nullptr; //actually a wall, disconnect!
					}
				}
			}
		}	
	}
}

NavigationGrid::~NavigationGrid()	{
	delete[] allNodes;
}

bool NavigationGrid::FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) const {
	//need to work out which node 'from' sits in, and 'to' sits in
	int fromX = ((int)from.x / nodeSize);
	int fromZ = ((int)from.z / nodeSize);

	int toX = ((int)to.x / nodeSize);
	int toZ = ((int)to.z / nodeSize);

	if (fromX < 0 || fromX > gridWidth - 1 ||
		fromZ < 0 || fromZ > gridHeight - 1) {
		return false; //outside of map region!
	}

	if (toX < 0 || toX > gridWidth - 1 ||
		toZ < 0 || toZ > gridHeight - 1) {
		return false; //outside of map region!
	}

	GridNode* startNode = &allNodes[(fromZ * gridWidth) + fromX];
	GridNode* endNode	= &allNodes[(toZ * gridWidth) + toX];

	// TODO: This is very alloc heavy, can we avoid new?
	// TODO: Use a set for closedList and a priority queue for openList
	std::vector<SearchNode*>  openList;
	std::vector<SearchNode*>  closedList;
	auto freeLists = [&]() {
		for (auto n : openList) {
			delete n;
		}
		for (auto n : closedList) {
			delete n;
		}
	};

	openList.emplace_back(new SearchNode{
		nullptr,
		startNode,
		0,
		0
	});

	SearchNode* currentBestNode = nullptr;

	while (!openList.empty()) {
		currentBestNode = RemoveBestNode(openList);

		if (currentBestNode->node == endNode) {			//we've found the path!
			SearchNode* node = currentBestNode;
			while (node != nullptr) {
				outPath.PushWaypoint(node->node->position);
				node = node->parent;
			}
			freeLists();
			return true;
		}
		else {
			for (int i = 0; i < 4; ++i) {
				GridNode* neighbour = currentBestNode->node->connected[i];
				if (!neighbour) { //might not be connected...
					continue;
				}	
				bool inClosed	= NodeInList(neighbour, closedList);
				if (inClosed) {
					continue; //already discarded this neighbour...
				}

				float h = Heuristic(neighbour, endNode);				
				float g = currentBestNode->currentCost + currentBestNode->node->costs[i];
				float f = h + g;

				SearchNode* openNode = FindNode(neighbour, openList);
				bool isNew = openNode == nullptr;

				if (isNew) { //first time we've seen this neighbour
					openNode = new SearchNode();
					openNode->node = neighbour;
					openList.emplace_back(openNode);
				}
				// Haven't seen the neighbour or it's a better route
				if (isNew || f < openNode->currentPlusHeuristic) {
					openNode->parent = currentBestNode;
					openNode->currentCost = g;
					openNode->currentPlusHeuristic = f;
				}
			}
			closedList.emplace_back(currentBestNode);
		}
	}
	freeLists();
	return false; //open list emptied out with no path!
}

bool NavigationGrid::NodeInList(SearchNode* n, std::vector<SearchNode*>& list) const {
	auto i = std::find(list.begin(), list.end(), n);
	return i != list.end();
}

bool NavigationGrid::NodeInList(GridNode* n, std::vector<SearchNode*>& list) const {
	return FindNode(n, list) != nullptr;
}

SearchNode* NavigationGrid::FindNode(GridNode* n, std::vector<SearchNode*>& list) const {
	for (auto i : list) {
		if (i->node == n) {
			return i;
		}
	}
	return nullptr;
}

SearchNode*  NavigationGrid::RemoveBestNode(std::vector<SearchNode*>& list) const {
	auto bestI = list.begin();
	auto bestNode = *list.begin();

	for (auto i = list.begin(); i != list.end(); ++i) {
		if ((*i)->currentPlusHeuristic < bestNode->currentPlusHeuristic) {
			bestNode	= (*i);
			bestI		= i;
		}
	}
	list.erase(bestI);

	return bestNode;
}

float NavigationGrid::Heuristic(GridNode* hNode, GridNode* endNode) const {
	return Vector::Length(hNode->position - endNode->position);
}