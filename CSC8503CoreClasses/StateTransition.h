#pragma once

namespace NCL {
	namespace CSC8503 {
		class State;
		using StateTransitionFunction = std::function<bool()>;
		class StateTransition	{
		public:
			// Use a lambda function instead of subclassing to reduce boilerplate code
			// Lambdas can capture the local scope for things such as the game object
			StateTransition(State* source, State* dest, StateTransitionFunction f) {
				sourceState		 = source;
				destinationState = dest;
				function		 = f;
			}

			bool CanTransition() const {
				return function();
			}

			State* GetDestinationState()  const {
				return destinationState;
			}

			State* GetSourceState() const {
				return sourceState;
			}

		protected:
			State * sourceState;
			State * destinationState;
			StateTransitionFunction function;
		};
	}
}

