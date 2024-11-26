#include "BehaviourParallel.h"

BehaviourState BehaviourParallel::Execute(float dt) {
    bool allFailed = true;
    for (auto& i : childNodes) {
        BehaviourState nodeState = i->Execute(dt);
        switch (nodeState) {
            case Failure:
                // Optionally fail as soon as one child fails
                if (failFast) {
                    return Failure;
                }
                break;
            case Success:
				return Success;
            case Ongoing:
                // At least one child is still running, don't fail yet
                allFailed = false;
            default:
				break;
        }
    }

    return allFailed ? Failure : Ongoing;
}
