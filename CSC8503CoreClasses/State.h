#pragma once

#include "StateMachine.h"

namespace NCL {
	namespace CSC8503 {
		class State {
		public:
			virtual ~State() {}

			virtual void Update(float dt) = 0;
		};

		// std::function: Like a function pointer, but without the demon summoning
		using StateUpdateFunction = std::function<void(float)>;
		class FunctionState : public State {
		public:
			// A basic state will just have an update function and nothing else
			// We could make this virtual or a generic function pointer if we wanted to add more functionality
			// E.g., StatelessState<Func> : public State for a state with just an update function
			FunctionState(StateUpdateFunction someFunc) {
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
			SubStateMachine(StateMachine* machine) {
				this->machine = machine;
			}

			~SubStateMachine() override {
				delete machine;
			}

			void Update(float dt) override {
				machine->Update(dt);
			}
		protected:
			StateMachine* machine;
		};
	}
}