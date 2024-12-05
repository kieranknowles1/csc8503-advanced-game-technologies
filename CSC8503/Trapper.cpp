#include "Trapper.h"

#include "Debug.h"

#include "AABBVolume.h"
#include "RenderObject.h"
#include "StateTransition.h"

#include "GameWorld.h"
#include "Ray.h"

namespace NCL::CSC8503 {
    namespace {
        class IdleState : public State {
        public:
            IdleState(StateMachine* parent, Rng& rng, MoveToTargetState* wander, Trapper* owner, float duration)
				: State(parent)
                , rng(rng)
                , wander(wander)
                , waitRemaining(0)
                , duration(duration)
				, owner(owner) {}

            ~IdleState() override {}

            void Update(float dt) override {
                waitRemaining -= dt;
                // Slow ourselves down
                Vector3 velocity = owner->GetPhysicsObject()->GetLinearVelocity();
                if (Vector::Length(velocity) > 0.1f) {
					owner->GetPhysicsObject()->AddForce(-velocity * 50 * dt);
				}
            }

            void OnBegin() override {
                std::cout << "Begin Idle for " << duration << std::endl;
                waitRemaining = duration;
			}

            void createFromTransition() {
                // this->wander
                // Trigger: TimePassed >= WaitDuration
                // Action: Set wander target to a random floor node
                parent->AddTransition(new StateTransition(this, wander, [this]()->bool {
                    return waitRemaining <= 0;
                }));
            }

            void createToTransition() {
                // wander->this
                // Trigger: DistanceToTarget < DistanceThreshold
                parent->AddTransition(new StateTransition(wander, this, [this]()->bool {
                    auto distance = Vector::Length(wander->getTarget() - owner->GetTransform().GetPosition());
                    if (distance < wander->getDistanceThreshold()) {
						std::cout << "Waiting" << std::endl;
						return true;
					}
                    return false;
                }));
            }
        protected:
            Rng& rng;
            Trapper* owner;
            MoveToTargetState* wander;
            float duration;
            float waitRemaining;

        };

        StateMachine* createStateMachine(Trapper* owner, NavigationGrid* nav, Rng& rng, GameWorld* world) {
            StateMachine* machine = new StateMachine();
            StateMachine* chase = new StateMachine();
            StateMachine* idle = new StateMachine();

            RandomMoveState* wander = new RandomMoveState(idle, owner, nav, rng);

            IdleState* wait = new IdleState(idle, rng, wander, owner, Trapper::WaitDuration);
            wait->createFromTransition();
            wait->createToTransition();

            IdleState* instantWait = new IdleState(idle, rng, wander, owner, 0);
            instantWait->createFromTransition();

            idle->AddState(wait);
            idle->AddState(instantWait);
            idle->setStartingState(instantWait);
            idle->AddState(wander);

            auto chaseFollow = new ChaseState(chase, owner, nav, world);
            chase->AddState(chaseFollow);
            chase->setStartingState(chaseFollow);


            auto idleState = new SubStateMachine(machine, idle);
            auto chaseState = new SubStateMachine(machine, chase);
            machine->AddState(idleState);
            machine->setStartingState(idleState);
            machine->AddState(chaseState);

            machine->AddTransition(new StateTransition(idleState, chaseState, [world, owner, chaseFollow]()->bool {
                TaggedObjects::iterator begin; TaggedObjects::iterator end;
                world->getTaggedObjects(GameObject::Tag::Player, begin, end);

                for (auto i = begin; i != end; i++) {
                    GameObject* player = i->second;
                    // TODO: Only chase if trespassing
                    if (world->hasLineOfSight(owner, player)) {
                        chaseFollow->setTargetObject(player);
                        return true;
                    }
                }
                return false;
            }));

            // TODO: Return to wander when reaching last known position
            // TODO: Penalty if the player is caught

            return machine;
        }
    }

    Vector3 RandomMoveState::pickTarget() {
        std::uniform_int_distribution<int> dist(0, navMap->getNodeCount());
        GridNode* node;
        do {
            node = navMap->getNode(dist(rng));
        } while (node->type != FLOOR_NODE);
        return node->position;
    }

    bool ChaseState::shouldRepickTarget() {
        // Update the target if we have line of sight
        // If not, keep going to the last known position
        Vector3 direction = Vector::Normalise(
            targetObject->GetTransform().GetPosition()
            - owner->GetTransform().GetPosition()
        );
        Ray ray(owner->GetTransform().GetPosition(), direction);
        RayCollision closest;


        bool hit = world->Raycast(ray, closest, true, owner);
        return hit && closest.node == targetObject;
    }

    Vector3 ChaseState::pickTarget() {
        return targetObject->GetTransform().GetPosition();
    }

    Trapper::Trapper(
        Rng& rng,
        Rendering::Mesh* mesh,
        Rendering::Shader* shader,
        NavigationGrid* nav,
        GameWorld* world
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
            .SetPosition(Vector3(60, 5, 45));

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

        stateMachine = createStateMachine(this, nav, rng, world);
    }

    Vector3 MoveToTargetState::getNextWaypoint() {
        if (path.empty()) {
            std::cout << "Finding path" << std::endl;
            bool ok = navMap->FindPath(owner->GetTransform().GetPosition(), target, path);
            if (!ok) {
				std::cout << "Failed to find path" << std::endl;
				return owner->GetTransform().GetPosition();
			}
#ifdef _DEBUG
            NavigationPath path2;
            navMap->FindPath(owner->GetTransform().GetPosition(), target, path2);
            Vector3 next;
            Vector3 prev = owner->GetTransform().GetPosition();
            while (path2.PopWaypoint(next)) {
				Debug::DrawLine(prev, next, Vector4(1, 0, 0, 1));
				prev = next;
			}
#endif // _DEBUG
        }
        Vector3 next;
        while (path.PopWaypoint(next) && Vector::Length(next - owner->GetTransform().GetPosition()) < waypointThreshold) {}
        return next;
	}

    void MoveToTargetState::Update(float dt)
    {
        if (shouldRepickTarget()) {
            setTarget(pickTarget());
        }

        float waypointDistance = Vector::Length(owner->GetTransform().GetPosition() - nextWaypoint);
        if (waypointDistance < waypointThreshold) {
			nextWaypoint = getNextWaypoint();
		}

        Vector3 direction = nextWaypoint - owner->GetTransform().GetPosition();
        direction.y = 0;
        direction = Vector::Normalise(direction);
        Vector3 facing = owner->GetTransform().GetOrientation() * Vector3(0, 0, 1);
        Vector3 deltaAngle = Vector::Cross(facing, direction);

        // TODO: CMake option for this
//#define AI_USE_FORCES
#ifdef AI_USE_FORCES
        // Apply a force to turn us towards the next waypoint
        // Stay upright

        if (Vector::Length(deltaAngle) > 0.1f) {
            owner->GetPhysicsObject()->AddTorque(-deltaAngle * 100 * dt);
        }

        // Move towards the waypoint
        // desiredVelocity = vector towards waypoint * speed
        Vector3 desiredVelocity = direction * speed;
        Vector3 actualVelocity = owner->GetPhysicsObject()->GetLinearVelocity();

        // What angle will get us closer to the desired velocity?
        Vector3 deltaVelocity = desiredVelocity - actualVelocity;
        // Apply a force that will reduce the delta between the desired and actual velocity
        Vector3 forceDirection = Vector::Normalise(deltaVelocity);

        owner->GetPhysicsObject()->AddForce(forceDirection * 100 * dt);
#else
        // Directly set the velocity
        // Quaternions are nasty, so only use the yaw angle. We don't want our actors to be drunk anyway
        // C++ works in radians, convert to degrees as required for Rich's code
        float yawAngleDegrees = atan2(direction.x, direction.z) * 180 / PI;
        yawAngleDegrees += 180;
        Quaternion q = Quaternion::EulerAnglesToQuaternion(0, yawAngleDegrees, 0);
        owner->GetTransform().SetOrientation(q);
        owner->GetTransform().SetPosition(owner->GetTransform().GetPosition() + direction * speed * dt);
#endif

        Debug::DrawLine(owner->GetTransform().GetPosition(), nextWaypoint, Vector4(0, 1, 0, 1));
    }
}
