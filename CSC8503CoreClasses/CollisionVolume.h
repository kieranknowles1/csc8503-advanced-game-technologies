#pragma once
namespace NCL {
	enum class VolumeType {
		AABB     = 1 << 0,
		OBB      = 1 << 1,
		Sphere   = 1 << 2,
		Mesh     = 1 << 3,
		Capsule  = 1 << 4,
		Compound = 1 << 5,
		Plane    = 1 << 6,
	};

	class CollisionVolume
	{
	public:
		CollisionVolume(VolumeType type)
			: type(type) {}
		~CollisionVolume() {}

		VolumeType type;
	};
}
