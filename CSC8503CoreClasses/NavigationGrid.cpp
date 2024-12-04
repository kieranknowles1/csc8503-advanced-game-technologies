#include "NavigationGrid.h"

#include <cassert>
#include <queue>
#include <fstream>

#include "Assets.h"
#include "Debug.h"

using namespace NCL;
using namespace CSC8503;

const int LEFT_NODE		= 0;
const int RIGHT_NODE	= 1;
const int TOP_NODE		= 2;
const int BOTTOM_NODE	= 3;

NavigationGrid::NavigationGrid()	{
	nodeSize	= 0;
	gridWidth	= 0;
	gridHeight	= 0;
	allNodes	= nullptr;
}

NavigationGrid::NavigationGrid(const std::string&filename, Vector3 offset) : NavigationGrid() {
	std::ifstream infile(Assets::DATADIR + filename);

	this->offset = offset;
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
			n.position = Vector3((float)(x * nodeSize), 0, (float)(y * nodeSize)) + offset;
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
	// The position of a node refers to its centre,
	// so we need to transform from/to by the offset minus half the node size
	// to get a grid aligned with the top-left of each node
	// *-------+ <- TopLeft of bounding box
	// |       |
	// |   *   | <- node position
	// |       | <- Any from/to in this box will return this node
	// +-------+

	// Fun fact: ASCII art line drawings in comments are less cursed than
	// UML diagrams in comments


	Vector2 offsetPlusHalfSize =
		Vector2(offset.x, offset.z)
		- Vector2(nodeSize / 2, nodeSize / 2);

	int fromX = ((int)(from.x - offsetPlusHalfSize.x) / nodeSize);
	int fromZ = ((int)(from.z - offsetPlusHalfSize.y) / nodeSize);

	int toX = ((int)(to.x - offsetPlusHalfSize.x) / nodeSize);
	int toZ = ((int)(to.z - offsetPlusHalfSize.y) / nodeSize);

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

	Debug::DrawLine(startNode->position, endNode->position, Debug::GREEN);

	// All nodes we've seen, with their best parent and cost
	// See `closedNodes` for nodes we've already expanded
	std::map<const GridNode*, SearchNode> seenNodes;
	std::set<const GridNode*> closedNodes;
	// The open list of nodes to expand. This may contain nodes that we found a better route to, so check the seenNodes map
	// This is faster than a vector in this case as finding the best node is O(log n) instead of O(n)
	// The priority queue is a max heap by default, so explicitly create it as a min heap
	std::priority_queue<SearchNode, std::vector<SearchNode>, std::greater<SearchNode>> openList;

	// Push a node onto the open list, if we haven't already seen a better route
	auto pushIfBetter = [&](const SearchNode& node) {
		auto existing = seenNodes.find(node.node);
		if (existing != seenNodes.end() && existing->second.currentPlusHeuristic > node.currentPlusHeuristic) {
			// The current route is better
			return;
		}
		openList.push(node);
		seenNodes[node.node] = node;
	};

	// Pop the best node from the open list, skipping nodes that have already been closed
	// Returns SearchNode::empty() if the open list is empty
	auto popNode = [&]() {
		while (!openList.empty()) {
			auto node = openList.top();
			openList.pop();
			// We can't remove arbitrary nodes from the priority queue,
			// so check that the GridNode hasn't been closed
			bool closed = closedNodes.find(node.node) != closedNodes.end();
			if (!closed) {
				return node;
			}
		}
		return SearchNode::empty();
	};

	pushIfBetter(SearchNode{
		nullptr, // parent
		startNode, // node
		0, // currentCost
		0, // costPlusHeuristic
	});

	while (!openList.empty()) {
		auto currentBestNode = popNode();
		if (currentBestNode.isEmpty()) {
			break; // Couldn't find a path
		}

		if (currentBestNode.node == endNode) {			//we've found the path!
			SearchNode node = currentBestNode;
			while (node.parent != nullptr) {
				outPath.PushWaypoint(node.node->position);
				node = seenNodes.find(node.parent)->second;
			}
			// Add the start node
			outPath.PushWaypoint(node.node->position);
			return true;
		}
		else {
			for (int i = 0; i < 4; ++i) {
				GridNode* neighbour = currentBestNode.node->connected[i];
				if (!neighbour) { //might not be connected...
					continue;
				}

				if (closedNodes.find(neighbour) != closedNodes.end()) {
					continue; //already discarded this neighbour...
				}

				float h = Heuristic(neighbour, endNode);
				float g = currentBestNode.currentCost + currentBestNode.node->costs[i];
				float f = h + g;

				pushIfBetter(SearchNode{
					currentBestNode.node, // parent
					neighbour, // node
					g, // currentCost
					f, // costPlusHeuristic
				});
			}
			// Mark this node as closed
			closedNodes.insert(currentBestNode.node);
			// Track the best route to this node
			seenNodes[currentBestNode.node] = currentBestNode;
		}
	}
	return false; //open list emptied out with no path!
}

float NavigationGrid::Heuristic(GridNode* hNode, GridNode* endNode) const {
	return Vector::Length(hNode->position - endNode->position);
}

void NavigationGrid::debugDraw()
{
	auto drawConnection = [&](GridNode* a, GridNode* b) {
		if (a && b) {
			Debug::DrawLine(a->position, b->position, Debug::WHITE);
		}
	};

	int numNodes = gridWidth * gridHeight;
	for (int i = 0; i < numNodes; i++) {
		GridNode& node = allNodes[i];
		if (node.type == WALL_NODE) {
			Vector3 halfSize(nodeSize / 2, 0, nodeSize / 2);
			Vector3 topLeft = node.position - halfSize;
			Vector3 bottomRight = node.position + halfSize;
			Vector3 topRight(topLeft.x, 0, bottomRight.z);
			Vector3 bottomLeft(bottomRight.x, 0, topLeft.z);

			Debug::DrawLine(topLeft, topRight, Debug::RED);
			Debug::DrawLine(topRight, bottomRight, Debug::RED);
			Debug::DrawLine(bottomRight, bottomLeft, Debug::RED);
			Debug::DrawLine(bottomLeft, topLeft, Debug::RED);
		} else {
			drawConnection(&node, node.connected[TOP_NODE]);
			drawConnection(&node, node.connected[RIGHT_NODE]);
			// The neighbour will draw the connection to the bottom and left as we are its top or right neighbour
		}
	}
}
