#include "Trapper.h"

#include "Debug.h"

#include "AABBVolume.h"
#include "RenderObject.h"
#include "StateTransition.h"

namespace NCL::CSC8503 {
    namespace {
        StateMachine* createStateMachine(GameObject* owner, NavigationGrid* nav, Rng& rng) {
            StateMachine* machine = new StateMachine();
            StateMachine* chase = new StateMachine();
            StateMachine* idle = new StateMachine();

            WanderState* wander = new WanderState(idle, owner, nav);
            float* waitRemaining = new float(0); // TODO: Memory leak, should probably use a functor object
            State* wait = new FunctionState(idle, [=](float dt) {
				*waitRemaining -= dt;

                // Slow ourselves down
                Vector3 velocity = owner->GetPhysicsObject()->GetLinearVelocity();
                if (Vector::Length(velocity) > 0.1f) {
                    owner->GetPhysicsObject()->AddForce(-velocity * 50 * dt);
                }
			});

            idle->AddTransition(new StateTransition(wait, wander, [waitRemaining,nav,wander,&rng]() {
                if (*waitRemaining > 0) return false;

                std::uniform_int_distribution<int> dist(0, nav->getNodeCount());
                int i;
                do {
					i = dist(rng);
				} while (nav->getNode(i)->type != FLOOR_NODE);
                wander->setTarget(nav->getNode(i)->position);
                std::cout << "Wandering to " << nav->getNode(i)->position.x << ", " << nav->getNode(i)->position.y << ", " << nav->getNode(i)->position.z << std::endl;
				return true;
            }));

            idle->AddTransition(new StateTransition(wander, wait, [waitRemaining, wander, owner]()->bool {
                float distance = Vector::Length(wander->getTarget() - owner->GetTransform().GetPosition());
                if (distance < wander->getDistanceThreshold()) {
					std::cout << "Waiting" << std::endl;
                    *waitRemaining = Trapper::WaitDuration;
					return true;
				}
                return false;
            }));

            idle->AddState(wait);
            idle->setStartingState(wait);
            idle->AddState(wander);

            // TODO: Temp
            float* cooldown = new float(0);
            auto dummy = new FunctionState(chase, [](float dt) {
                std::cout << "Chasing" << std::endl;
            });
            chase->AddState(dummy);
            chase->setStartingState(dummy);



            auto idleState = new SubStateMachine(machine, idle);
            auto chaseState = new SubStateMachine(machine, chase);
            machine->AddState(idleState);
            machine->setStartingState(idleState);
            machine->AddState(chaseState);

            machine->AddTransition(new StateTransition(idleState, chaseState, [cooldown]()->bool {
                *cooldown -= 0.01f;
				if (*cooldown > 0) return false;
				*cooldown = 10;
                std::cout << "Idle -> Chase" << std::endl;
				return true;
			}));
            chase->AddTransition(new StateTransition(dummy, idleState, [cooldown]()->bool {
                *cooldown -= 0.01f;
				if (*cooldown > 0) return false;
				*cooldown = 10;
                std::cout << "Dummy -> Idle" << std::endl;
				return true;
			}));

            return machine;
        }
    }

    Trapper::Trapper(
        Rng& rng,
        Rendering::Mesh* mesh,
        Rendering::Shader* shader,
        NavigationGrid* nav
    ) {
        navMap = nav;

        float scale = 3.0f;
        //SetBoundingVolume(new SphereVolume(scale));
        // OBB volumes are janky, so use an AABB for now
        SetBoundingVolume(new AABBVolume(
            Vector3(0.3f, 0.9f, 0.3f) * scale
        ));
        GetTransform()
            .SetScale(Vector3(scale, scale, scale))
            .SetPosition(Vector3(30, 5, 45));

        SetRenderObject(new RenderObject(
            &GetTransform(),
            mesh,
            nullptr, // no texture
            shader
        ));

        SetPhysicsObject(new PhysicsObject(
            &GetTransform(),
            GetBoundingVolume()
        ));
        GetPhysicsObject()->InitCubeInertia();

        stateMachine = createStateMachine(this, nav, rng);
    }

    Vector3 WanderState::getNextWaypoint() {
        if (path.empty()) {
            std::cout << "Finding path" << std::endl;
            navMap->FindPath(owner->GetTransform().GetPosition(), target, path);
        }
        Vector3 next;
        path.PopWaypoint(next);
        return next;
	}

    void WanderState::Update(float dt)
    {
        float waypointDistance = Vector::Length(owner->GetTransform().GetPosition() - nextWaypoint);
        if (waypointDistance < waypointThreshold) {
			nextWaypoint = getNextWaypoint();
		}

        // Apply a force to turn us towards the next waypoint
        Vector3 direction = nextWaypoint - owner->GetTransform().GetPosition();
        // Stay upright
        direction.y = 0;
        direction = Vector::Normalise(direction);
        
        Vector3 facing = owner->GetTransform().GetOrientation() * Vector3(0, 0, 1);
        Vector3 deltaAngle = Vector::Cross(facing, direction);
        if (Vector::Length(deltaAngle) > 0.1f) {
            owner->GetPhysicsObject()->AddTorque(-deltaAngle * 100 * dt);
        }

        // Move towards the waypoint
        Vector3 desiredVelocity = direction * speed;
        Vector3 actualVelocity = owner->GetPhysicsObject()->GetLinearVelocity();

        // What angle will get us closer to the desired velocity?
        Vector3 deltaVelocity = desiredVelocity - actualVelocity;
        Vector3 forceDirection = Vector::Normalise(deltaVelocity);

        owner->GetPhysicsObject()->AddForce(forceDirection * 100 * dt);

        Debug::DrawLine(owner->GetTransform().GetPosition(), nextWaypoint, Vector4(0, 1, 0, 1));
    }
}
