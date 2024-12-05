#pragma once
#include "CollisionVolume.h"

namespace NCL {
    class CapsuleVolume : public CollisionVolume
    {
    public:
        CapsuleVolume(float halfHeight, float radius) : CollisionVolume(VolumeType::Capsule) {
            this->halfHeight    = halfHeight;
            this->radius        = radius;
        };
        ~CapsuleVolume() {

        }
        float GetRadius() const {
            return radius;
        }

        float GetHalfHeight() const {
            return halfHeight;
        }

    protected:
        float radius;
        float halfHeight;
    };
}

