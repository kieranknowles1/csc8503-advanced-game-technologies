#include "StateMachine.h"

#include <cassert>

#include "State.h"
#include "StateTransition.h"

using namespace NCL::CSC8503;

StateMachine::StateMachine()	{
	activeState = nullptr;
}

StateMachine::~StateMachine()	{
	for (auto& i : allStates) {
		delete i;
	}
	for (auto& i : allTransitions) {
		delete i.second;
	}
}

void StateMachine::AddState(State* s) {
	allStates.emplace_back(s);
}

void StateMachine::AddTransition(StateTransition* t) {
	allTransitions.insert(std::make_pair(t->GetSourceState(), t));
}

void StateMachine::Update(float dt) {
	assert(activeState && "No active state in state machine, did you set the initial state?");
	activeState->Update(dt);
	
	//Get the transition set starting from this state node;
	std::pair<TransitionIterator, TransitionIterator> range = allTransitions.equal_range(activeState);

	// If any transitions have been triggered, then we should change state
	for (auto& i = range.first; i != range.second; ++i) {
		if (i->second->CanTransition(dt)) {
			State* newState = i->second->GetDestinationState();

			if (newState->getParent() == this) {
				// Going from one state to another within the same state machine
				// The new state could be a sub-machine
				setState(newState);
			}
			else if (newState->getParent() == this->parent) {
				// Going from a child state machine to a parent state machine
				this->parent->setState(newState); // Update the state in the parent
				setState(newState); // Update the state in the child
			}
			else {
				// The new state is disconnected from the current state machine
				// Something has gone wrong
				throw std::runtime_error("State machine transitioned to a state that is not connected to the current state machine");
			}
		}
	}
}

void StateMachine::setState(State* s) {
	if (s == activeState) return;

	activeState->OnEnd();
	activeState = s;
	activeState->OnBegin();
}