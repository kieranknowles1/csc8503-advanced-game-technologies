#pragma once

#include "BehaviourNode.h"

// Behaviour node that inverts the result of its child
class BehaviourInverter : public BehaviourNode {
public:
    BehaviourInverter(const std::string& nodeName, BehaviourNode* child) : BehaviourNode(nodeName) {
        childNode = child;
    }
    ~BehaviourInverter() {
        delete childNode;
    }

    BehaviourState Execute(float dt) override {
        BehaviourState childState = childNode->Execute(dt);
        switch (childState) {
            case Success:
                return Failure;
            case Failure:
                return Success;
            default:
                return childState;
        }
    }

    void Reset() override {
        BehaviourNode::Reset();
        childNode->Reset();
    }

protected:
    BehaviourNode* childNode;
};
