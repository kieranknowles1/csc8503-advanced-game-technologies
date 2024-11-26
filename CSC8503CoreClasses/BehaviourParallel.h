#pragma once
#include "BehaviourNodeWithChildren.h"

// Parallel node. Executes all children at once. Succeeds once
// any child succeeds by default. Fails if all children fail
// or one if `failFast` is true.
class BehaviourParallel : public BehaviourNodeWithChildren {
public:
	BehaviourParallel(
		const std::string& nodeName,
		bool failFast = false
	)
		: BehaviourNodeWithChildren(nodeName)
		, failFast(failFast)
	{}

	~BehaviourParallel() {}

	BehaviourState Execute(float dt) override;

protected:
	// If true, the node will fail as soon as one child fails.
	bool failFast = false;
};
