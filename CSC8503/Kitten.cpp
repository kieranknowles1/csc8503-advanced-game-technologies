#include "Kitten.h"

#include "GameWorld.h"
#include "State.h"
#include "StateTransition.h"

#include "RenderObject.h"

namespace NCL::CSC8503 {
    // Apply an impulse to push an object towards a target velocity
    // Ignores the y axis
    // Force should be scaled by dt
    static void pushTowardsVelocity(GameObject* obj, Vector3 targetVelocity, float force) {
        Vector3 delta = targetVelocity - obj->GetPhysicsObject()->GetLinearVelocity();
        delta.y = 0;

        if (delta != Vector3()) {
            Vector3 forceDir = Vector::Normalise(delta);
            obj->GetPhysicsObject()->AddForce(forceDir * force);

            // Delta from target
            Debug::DrawLine(obj->GetTransform().GetPosition(), obj->GetTransform().GetPosition() + delta, Debug::GREEN);
            // Target velocity vector
            Debug::DrawLine(obj->GetTransform().GetPosition(), obj->GetTransform().GetPosition() + targetVelocity, Debug::RED);
            // Current velocity vector
            Debug::DrawLine(obj->GetTransform().GetPosition(), obj->GetTransform().GetPosition() + obj->GetPhysicsObject()->GetLinearVelocity(), Debug::MAGENTA);
        }
    }

    static StateMachine* createStateMachine(Kitten* kitten) {
        StateMachine* sm = new StateMachine();
        // TODO: Encapsulate this properly
        GameObject** followedPlayer = new GameObject*;
        auto idle = new FunctionState(sm, [=](float dt) {
            pushTowardsVelocity(kitten, Vector3(), kitten->getForce() * dt);
        });

        auto follow = new FunctionState(sm, [=](float dt) {
            Vector3 kPos = kitten->GetTransform().GetPosition();
            Vector3 pPos = (*followedPlayer)->GetTransform().GetPosition();
            Vector3 offset = pPos - kPos;
            offset.y = 0; // Ignore height

            // Move towards player, but stop when close enough5
            bool closeEnough = Vector::Length(offset) < kitten->getTargetDistance();
            Vector3 targetVelocity = closeEnough
                ? Vector3()
                : Vector::Normalise(offset) * kitten->getSpeed();
            Vector3 deltaVelocity = targetVelocity - kitten->GetPhysicsObject()->GetLinearVelocity();
            deltaVelocity.y = 0; // Ignore height
            // Scale based on distance if approaching, don't scale if we're close and want to stop
            float forceScaling = closeEnough
                ? 1.0f
                : Vector::Length(offset) / kitten->getFollowEndDistance();

            // Apply acceleration to reduce delta velocity, sort of a constraint
            if (deltaVelocity != Vector3()) {
                pushTowardsVelocity(kitten, targetVelocity, kitten->getForce() * dt * forceScaling);
            }

            // atan2 doesn't need a normalised vector
            float yaw = RadiansToDegrees(atan2(offset.x, offset.z));
            kitten->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(0, yaw, 0));
        });

        sm->AddState(idle);
        sm->setStartingState(idle);
        sm->AddState(follow);

        sm->AddTransition(new FunctionStateTransition(idle, follow, [=](float dt) {
            TaggedObjects::iterator begin; TaggedObjects::iterator end;
            kitten->GetWorld()->getTaggedObjects(GameObject::Tag::Player, begin, end);
            for (auto i = begin; i != end; i++) {
                GameObject* player = i->second;
                bool los = kitten->GetWorld()->hasLineOfSight(
                    kitten, player, kitten->getSightRange()
                );
                if (los) {
                    *followedPlayer = player;
                    std::cout << "Following " << player->GetName() << std::endl;
                    return true;
                }
            }
            return false;
        }));

        sm->AddTransition(new FunctionStateTransition(follow, idle, [=](float dt) {
            return !kitten->GetWorld()->hasLineOfSight(
                kitten, *followedPlayer, kitten->getFollowEndDistance()
            );
        }, 3.0f));

        return sm;
    }

    Kitten::Kitten(Mesh* mesh, Shader* shader, Texture* texture, GameWorld* world) {
        stateMachine = createStateMachine(this);
        this->world = world;

        SetRenderObject(new RenderObject(&transform, mesh, texture, shader));
        SetBoundingVolume(new SphereVolume(1.0f));
        SetPhysicsObject(new PhysicsObject(&transform, boundingVolume));

        GetPhysicsObject()->SetAngularDamping(0.5f);
        GetPhysicsObject()->SetLinearDamping(0.5f);
    }
}
