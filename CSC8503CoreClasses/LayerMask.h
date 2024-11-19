#pragma once
#include <limits>
#include <initializer_list>

namespace NCL::CSC8503 {
	// A layer is a 32-bit integer, with each bit representing a different layer
	// Each object is assigned a layer, and can collide with any other object where the mask matches
	class LayerMask {
	public:
        // Strongly typed enum for layer usage
		// Abstracts away bit manipulation
		enum class Index {
			Default = 0,
			Spheres = 1,
		};

		// Default: Match anything
		constexpr LayerMask() : mask(std::numeric_limits<uint32_t>::max()) {}
		
		// Match only the listed layers
		// constexpr: can be evaluated at compile time
		constexpr LayerMask(std::initializer_list<Index> layers) : mask(0) {
			for (auto& layer : layers) {
				mask |= 1 << (int)layer;
			}
		}

		uint32_t get() { return mask; }
		void set(uint32_t m) { mask = m; }

		bool matches(Index layer) const {
			return (mask & (1 << (int)layer)) != 0;
		}
	private:
		uint32_t mask;
	};
}