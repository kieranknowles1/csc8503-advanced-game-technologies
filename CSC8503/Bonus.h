#pragma once

#include "GameObject.h"

namespace NCL::CSC8503 {
    class NetworkedGame;
    class Bonus : public GameObject {
    public:
        Bonus(NetworkedGame* game, int value)
            : game(game)
            , value(value) {
            setLayer(LayerMask::Index::Item);
        }

        void OnCollisionBegin(GameObject* other) override;

        int getValue() const {
			return value;
		}
        void setValue(int value) {
            this->value = value;
        }
    private:
        NetworkedGame* game;
        int value;
    };
};
