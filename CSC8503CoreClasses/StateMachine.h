#pragma once

namespace NCL {
	namespace CSC8503 {
		class State;
		class StateTransition;

		using TransitionContainer = std::multimap<State*, StateTransition*>;
		using TransitionIterator = TransitionContainer::iterator;

		class StateMachine	{
		public:
			StateMachine();
			virtual ~StateMachine(); //made it virtual!

			void AddState(State* s);
			void setStartingState(State* s) { activeState = s; }
			void AddTransition(StateTransition* t);

			virtual void Update(float dt); //made it virtual!

		protected:
			State * activeState;

			std::vector<State*> allStates;
			TransitionContainer allTransitions;
		};
	}
}