#pragma once

#include "GameObject.h"

namespace NCL::CSC8503 {
    class NetworkedGame;
    class Bonus : public GameObject {
    public:
        Bonus(NetworkedGame* game, int value)
            : game(game)
            , value(value) {}

        void OnCollisionBegin(GameObject* other) override;
    private:
        NetworkedGame* game;
        int value;
    };
};
