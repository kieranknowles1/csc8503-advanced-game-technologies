#pragma once

#include "StateMachine.h"
#include "State.h"
#include "Mesh.h"
#include "Shader.h"
#include "NavigationGrid.h"
#include "GameObject.h"

#include "Rng.h"

namespace NCL::CSC8503 {
    // State to wander towards a target
    // Completes when close enough to the target
    class WanderState : public State {
    public:
        WanderState(GameObject* owner, NavigationGrid* navMap)
            : owner(owner)
            , navMap(navMap) {}

        virtual void Update(float dt) override;

        void setTarget(Vector3 target) {
			this->target = target;
            path.Clear();
		}

        float getDistanceThreshold() const {
			return distanceThreshold;
		}
        Vector3 getTarget() const {
            return target;
        }
    private:
        Vector3 target;
        float distanceThreshold = 5.0f;

        GameObject* owner;
        NavigationGrid* navMap;
        NavigationPath path;
    };

    class Trapper : public GameObject {
        public:
            const constexpr static float WaitDuration = 5.0f;

            // TODO: How to give the client and server the same RNG state?
            Trapper(Rng& rng, Rendering::Mesh* mesh, Rendering::Shader* shader, NavigationGrid* nav);

            void OnUpdate(float dt) override {
				stateMachine->Update(dt);
			}
        protected:
            NavigationGrid* navMap;

            StateMachine* stateMachine;
    };
}
