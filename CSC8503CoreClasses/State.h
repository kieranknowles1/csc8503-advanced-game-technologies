#pragma once

namespace NCL {
	namespace CSC8503 {
		// std::function: Like a function pointer, but without the demon summoning
		using StateUpdateFunction = std::function<void(float)>;

		class  State		{
		public:
			State() {}
			// A basic state will just have an update function and nothing else
			// We could make this virtual or a generic function pointer if we wanted to add more functionality
			// E.g., StatelessState<Func> : public State for a state with just an update function
			State(StateUpdateFunction someFunc) {
				func		= someFunc;
			}
			void Update(float dt)  {
				if (func != nullptr) {
					func(dt);
				}
			}
		protected:
			StateUpdateFunction func;
		};
	}
}