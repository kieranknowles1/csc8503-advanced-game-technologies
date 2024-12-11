#pragma once

#include "GameObject.h"
#include "GameWorld.h"

namespace NCL::CSC8503 {
    class NetworkedGame;
    class Bonus : public GameObject {
    public:
        Bonus(NetworkedGame* game, int value, Rng& rng)
            : game(game)
            , value(value) {
            setLayer(LayerMask::Index::Item);
            // Random orientation so the starting direction has some variety
            std::uniform_real_distribution<float> dist(0, 360);
            GetTransform().SetOrientation(
                Quaternion::EulerAnglesToQuaternion(0, dist(rng), 0)
            );
        }

        void OnCollisionBegin(GameObject* other) override;
        void OnUpdate(float dt) override;

        int getValue() const {
			return value;
		}
        void setValue(int value) {
            this->value = value;
        }
    private:
        NetworkedGame* game;
        float spinSpeed = 90.0f;
        int value;
    };
};
