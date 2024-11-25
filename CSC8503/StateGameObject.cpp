#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"

using namespace NCL;
using namespace CSC8503;

StateGameObject::StateGameObject()
	: counter(0)
	, stateMachine(new StateMachine())
{
	// Lambdas can capture locals such as the `this` pointer
	State* stateA = new State([&](float dt)->void {
		counter += dt;
		MoveLeft(dt);
	});
	State* stateB = new State([&](float dt)->void {
		counter -= dt;
		MoveRight(dt);
	});
	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);
	stateMachine->setStartingState(stateA);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]()->bool {
		return counter > 3.0f;
	}));
	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]()->bool {
		return counter < 0.0f;
	}));
}

StateGameObject::~StateGameObject() {
	delete stateMachine;
}

void StateGameObject::Update(float dt) {
	stateMachine->Update(dt);
}

void StateGameObject::MoveLeft(float dt) {
	GetPhysicsObject()->AddForce({ -100, 0, 0 });
}

void StateGameObject::MoveRight(float dt) {
	GetPhysicsObject()->AddForce({ 100, 0, 0 });
}