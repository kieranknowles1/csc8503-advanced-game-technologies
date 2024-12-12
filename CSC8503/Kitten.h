#pragma once

#include "GameObject.h"
#include "NetworkPlayer.h"
#include "StateMachine.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "GameWorld.h"

namespace NCL::CSC8503 {
    class Kitten : public GameObject {
    public:
        Kitten(Mesh* mesh, Shader* shader, Texture* texture, NetworkedGame* game);

        Tag getTag() const override {
			return Tag::Kitten;
		}

        void OnUpdate(float dt) override {
            stateMachine->Update(dt);
        }

        float getSightRange() const {
            return sightRange;
        }
        float getFollowEndDistance() const {
            return followEndDistance;
        }
        float getSpeed() const {
            return speed;
        }
        float getTargetDistance() const {
            return targetDistance;
        }
        float getForce() const {
			return force;
		}
    private:
        float sightRange = 20.0f;
        float followEndDistance = sightRange + 5.0f;
        float speed = 5.0f; // mps
        float targetDistance = 8.0f;
        float force = 500.0f; // KG
        StateMachine* stateMachine;
    };
}
