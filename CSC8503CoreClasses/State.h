#pragma once

#include "StateMachine.h"

namespace NCL {
	namespace CSC8503 {
		class State {
		public:
			State(StateMachine* parent)
				: parent(parent) {}
			virtual ~State() {}

			virtual void OnBegin() {}
			virtual void OnEnd() {}
			virtual void Update(float dt) = 0;

			StateMachine* getParent() const {
				return parent;
			}

		protected:
			StateMachine* parent;
		};

		// std::function: Like a function pointer, but without the demon summoning
		using StateUpdateFunction = std::function<void(float)>;
		class FunctionState : public State {
		public:
			// A basic state will just have an update function and nothing else
			// We could make this virtual or a generic function pointer if we wanted to add more functionality
			// E.g., StatelessState<Func> : public State for a state with just an update function
			FunctionState(StateMachine* parent, StateUpdateFunction someFunc)
				: State(parent) {
				func = someFunc;
			}
			~FunctionState() override {}
			void Update(float dt) override {
				if (func != nullptr) {
					func(dt);
				}
			}
		protected:
			StateUpdateFunction func;
		};

		// A state that has its own state machine
		class SubStateMachine : public State {
		public:
			SubStateMachine(StateMachine* parent, StateMachine* child)
				: State(parent) {
				this->child = child;
				child->setParent(parent);
			}

			~SubStateMachine() override {
				delete child;
			}

			void Update(float dt) override {
				child->Update(dt);
			}

			void OnBegin() override {
				child->setState(child->getStartingState());
			}

			StateMachine* getChild() const {
				return child;
			}
		protected:
			StateMachine* child;
		};
	}
}