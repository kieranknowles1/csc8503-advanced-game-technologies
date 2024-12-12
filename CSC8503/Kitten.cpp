#include "Kitten.h"

#include "GameWorld.h"
#include "State.h"
#include "StateTransition.h"

#include "NetworkedGame.h"
#include "RenderObject.h"

namespace NCL::CSC8503 {
    // TODO: This is quite a bit of duplication with the trapper. Can't easily reuse class as the kitten doesn't
    // follow a navmesh
    void moveTowardsPosition(Kitten* kitten, Vector3 target, float minDistance, float dt) {
        Vector3 kPos = kitten->GetTransform().GetPosition();
        Vector3 offset = target - kPos;
        offset.y = 0; // Ignore height

        // Move towards player, but stop when close enough5
        bool closeEnough = Vector::Length(offset) < minDistance;
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
            kitten->GetPhysicsObject()->pushTowardsVelocity(targetVelocity, kitten->getForce() * dt * forceScaling);
        }

        // atan2 doesn't need a normalised vector
        float yaw = RadiansToDegrees(atan2(offset.x, offset.z));
        kitten->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(0, yaw, 0));
    }

    static StateMachine* createStateMachine(Kitten* kitten, NetworkedGame* game) {
        StateMachine* sm = new StateMachine();
        // TODO: Encapsulate this properly
        NetworkPlayer** followedPlayer = new NetworkPlayer*;
        Vector3* sleepPosition = new Vector3;

        auto idle = new FunctionState(sm, [=](float dt) {
            kitten->GetPhysicsObject()->pushTowardsVelocity(Vector3(), kitten->getForce() * dt);
        });

        auto follow = new FunctionState(sm, [=](float dt) {
            moveTowardsPosition(kitten, (*followedPlayer)->GetTransform().GetPosition(), kitten->getTargetDistance(), dt);
        });

        auto sleep = new FunctionState(sm, [=](float dt) {
            moveTowardsPosition(kitten, *sleepPosition, 0, dt);
        });

        sm->AddState(idle);
        sm->setStartingState(idle);
        sm->AddState(follow);

        sm->AddTransition(new FunctionStateTransition(idle, follow, [=](float dt) {
            TaggedObjects::iterator begin; TaggedObjects::iterator end;
            kitten->GetWorld()->getTaggedObjects(GameObject::Tag::Player, begin, end);
            for (auto i = begin; i != end; i++) {
                NetworkPlayer* player = (NetworkPlayer*)i->second;
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

        sm->AddTransition(new FunctionStateTransition(follow, sleep, [=](float dt) {
            bool inHome = (*followedPlayer)->isInHome();
            if (!inHome) return false;

            *sleepPosition = (*followedPlayer)->GetTransform().GetPosition();
            game->decrementRemainingKittens(kitten, *followedPlayer);
            return true;
        }));

        return sm;
    }

    Kitten::Kitten(Mesh* mesh, Shader* shader, Texture* texture, NetworkedGame* game) {
        stateMachine = createStateMachine(this, game);
        this->world = game->getWorld();

        SetRenderObject(new RenderObject(&transform, mesh, texture, shader));
        SetBoundingVolume(new SphereVolume(1.0f));
        SetPhysicsObject(new PhysicsObject(&transform, boundingVolume));

        GetPhysicsObject()->SetAngularDamping(0.5f);
        GetPhysicsObject()->SetLinearDamping(0.5f);
    }
}
