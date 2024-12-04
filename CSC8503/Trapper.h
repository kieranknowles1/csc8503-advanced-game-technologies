#pragma once

#include "StateMachine.h"
#include "State.h"
#include "Mesh.h"
#include "Shader.h"
#include "NavigationGrid.h"
#include "GameObject.h"

#include "Rng.h"

namespace NCL::CSC8503 {
    // Base class for states that move towards a target
    class MoveToTargetState : public State {
    public:
    MoveToTargetState(StateMachine* parent, GameObject* owner, NavigationGrid* navMap)
        : State(parent)
        , owner(owner)
        , navMap(navMap) {}

    void Update(float dt) override;
    void OnBegin() override {
        std::cout << "Begin moving to target" << std::endl;
        target = pickTarget();
        nextWaypoint = getNextWaypoint();
        path.Clear();
    }

    Vector3 getTarget() const {
        return target;
    }
    float getDistanceThreshold() const {
        return distanceThreshold;
    }

    protected:
        virtual Vector3 pickTarget() = 0;

        NavigationGrid* navMap;
        GameObject* owner;

        // Target speed in mps
        float speed = 5.0f;
        // Distance to the target at which the state completes
        float distanceThreshold = 10.0f;
        // Distance to a waypoint at which the next waypoint is selected
        float waypointThreshold = 8.0f;
    private:
        // Get the next waypoint to reach the target
        // Pops a waypoint or triggers a pathfind as needed
        Vector3 getNextWaypoint();

        Vector3 nextWaypoint;
        Vector3 target;

        NavigationPath path;
    };

    // Selects a random target within the navigation grid, and moves towards it
    class RandomMoveState : public MoveToTargetState {
    public:
        RandomMoveState(StateMachine* parent, GameObject* owner, NavigationGrid* navMap, Rng& rng)
            : MoveToTargetState(parent, owner, navMap)
            , rng(rng) {}
    protected:
        Vector3 pickTarget() override;
    private:
        Rng& rng;
    };

    class Trapper : public GameObject {
        public:
            const constexpr static float WaitDuration = 5.0f;

            // TODO: How to give the client and server the same RNG state?
            Trapper(Rng& rng, Rendering::Mesh* mesh, Rendering::Shader* shader, NavigationGrid* nav);

            void OnUpdate(float dt) override {
				stateMachine->Update(dt);

			}

            NavigationGrid* getNavMap() const {
				return navMap;
			}
        protected:
            NavigationGrid* navMap;

            StateMachine* stateMachine;
    };
}
