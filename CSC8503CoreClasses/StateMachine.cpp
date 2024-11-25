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
	if (activeState == nullptr) {
		activeState = s;
	}
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
		if (i->second->CanTransition()) {
			State* newState = i->second->GetDestinationState();
			activeState = newState;
		}
	}
}