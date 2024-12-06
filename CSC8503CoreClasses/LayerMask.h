#pragma once
#include <limits>
#include <initializer_list>

namespace NCL::CSC8503 {
	// A layer is a 32-bit integer, with each bit representing a different layer
	// Each object is assigned a layer, and can collide with any other object where the mask matches
	// Most of this class is constexpr and can be evaluated at compile time
	class LayerMask {
	public:
        // Strongly typed enum for layer usage
		// Abstracts away bit manipulation
		enum class Index {
			Default,
			// An item that can be picked up by the player
			Item,
			// An actor, either player or AI that moves on its own
			Actor,
		};

		// Default: Match anything
		constexpr LayerMask() : mask(std::numeric_limits<uint32_t>::max()) {}
		
		// Match only the listed layers
		constexpr LayerMask(std::initializer_list<Index> layers) : mask(0) {
			for (auto& layer : layers) {
				mask |= 1 << (int)layer;
			}
		}
		constexpr LayerMask operator~() const {
			return LayerMask(~mask);
		}

		uint32_t get() const { return mask; }
		void set(uint32_t m) { mask = m; }

		bool matches(Index layer) const {
			return (mask & (1 << (int)layer)) != 0;
		}
	private:
		constexpr LayerMask(uint32_t m) : mask(m) {}

		uint32_t mask;
	};
}