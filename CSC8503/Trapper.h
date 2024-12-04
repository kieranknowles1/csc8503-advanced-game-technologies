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
        WanderState(StateMachine* parent, GameObject* owner, NavigationGrid* navMap)
            : State(parent)
            , owner(owner)
            , navMap(navMap) {}

        virtual void Update(float dt) override;

        void setTarget(Vector3 target) {
			this->target = target;
            // Force a pathfind
            path.Clear();
            // Force a new waypoint to be selected
            nextWaypoint = owner->GetTransform().GetPosition();
		}

        float getDistanceThreshold() const {
			return distanceThreshold;
		}
        Vector3 getTarget() const {
            return target;
        }
    private:
        // Get the next waypoint to reach the target
        // Will trigger a pathfind if the current path is empty
        Vector3 getNextWaypoint();

        Vector3 target;
        Vector3 nextWaypoint;
        // Distance to target at which the state completes
        float distanceThreshold = 10.0f;
        // Distance to a waypoint at which the next waypoint is selected
        float waypointThreshold = 8.0f;
        // Target speed in mps
        float speed = 5.0f;

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
