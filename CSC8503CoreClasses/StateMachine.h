#pragma once

#include <cassert>
#include <vector>
#include <map>

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
			void setStartingState(State* s) { defaultState = s; activeState = s; }
			void AddTransition(StateTransition* t);

			virtual void Update(float dt); //made it virtual!
			void setState(State* s);

			State* getStartingState() const { return defaultState; }

			void setParent(StateMachine* p) {
				// Not tested this, will probably break
				assert(p->parent == nullptr && "Multiple nesting not supported");
				parent = p;
			}

		protected:
			StateMachine* parent = nullptr;
			State* defaultState = nullptr;
			State* activeState;

			std::vector<State*> allStates;
			TransitionContainer allTransitions;
		};
	}
}