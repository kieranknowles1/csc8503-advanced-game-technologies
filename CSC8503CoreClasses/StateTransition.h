#pragma once

namespace NCL {
	namespace CSC8503 {
		class State;
		using StateTransitionFunction = std::function<bool(float dt)>;

		// Abstract class in case we need something more complex than a lambda function and a hold time
		class StateTransition {
		public:
			StateTransition(State* source, State* dest) {
				sourceState		 = source;
				destinationState = dest;
			}

			virtual ~StateTransition() {}
			
			virtual bool CanTransition(float dt) = 0;

			State* GetDestinationState()  const {
				return destinationState;
			}

			State* GetSourceState() const {
				return sourceState;
			}
		protected:
			State* sourceState;
			State* destinationState;
		};

		class FunctionStateTransition : public StateTransition {
		public:
			// Use a lambda function instead of subclassing to reduce boilerplate code
			// Lambdas can capture the local scope for things such as the game object
			// If holdTime > 0, the condition must be met continuously for holdTime seconds before transitioning
			FunctionStateTransition(State* source, State* dest, StateTransitionFunction f, float holdTime = 0.0f)
				: StateTransition(source, dest)
				, function(f)
				, holdTime(holdTime)
				, currentHeldTime(0.0f) {
			}

			bool CanTransition(float dt) override {
				bool conditionMet = function(dt);
				if (!conditionMet) {
					currentHeldTime = 0.0f;
					return false;
				}

				currentHeldTime += dt;
				return currentHeldTime >= holdTime;
			}

		protected:
			float holdTime;
			float currentHeldTime;
			StateTransitionFunction function;
		};
	}
}

