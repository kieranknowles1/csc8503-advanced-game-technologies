#pragma once
#include <limits>

namespace NCL::CSC8503 {
	// A layer is a 32-bit integer, with each bit representing a different layer
	// Each object is assigned a layer, and can collide with any other object where the mask matches
	class LayerMask {
	public:
        // Strongly typed enum for layer usage
        // TODO: LayerMask::fromIndexes
		enum class Index {
			Default = 0,
			Spheres = 1,
		};

		// Default: Match anything
		LayerMask() : mask(std::numeric_limits<uint32_t>::max()) {}
		LayerMask(uint32_t mask) : mask(mask) {}
		uint32_t get() { return mask; }
		void set(uint32_t m) { mask = m; }

		bool matches(Index layer) const {
			return (mask & (1 << (int)layer)) != 0;
		}
	private:
		uint32_t mask;
	};
}