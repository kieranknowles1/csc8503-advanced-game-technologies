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

            WanderState* wander = new WanderState(owner, nav);
            float waitRemaining = 0;
            State* wait = new FunctionState([&](float dt) {
				waitRemaining -= dt;
			});

            idle->AddTransition(new StateTransition(wait, wander, [&waitRemaining,nav,wander,&rng]() {
                if (waitRemaining > 0) return false;
                waitRemaining = Trapper::WaitDuration;

                std::uniform_int_distribution<int> dist(0, nav->getNodeCount());
                int i;
                do {
					i = dist(rng);
				} while (nav->getNode(i)->type != FLOOR_NODE);
                wander->setTarget(nav->getNode(i)->position);
                waitRemaining = Trapper::WaitDuration;
				return true;
            }));

            idle->AddTransition(new StateTransition(wander, wait, [&waitRemaining, wander, owner]() {
                float distance = Vector::Length(wander->getTarget() - owner->GetTransform().GetPosition());
                return distance < wander->getDistanceThreshold();
            }));

            idle->AddState(wait);
            idle->setStartingState(wait);
            idle->AddState(wander);

            return idle;
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
        SetBoundingVolume(new AABBVolume(
            Vector3(0.3f, 0.9f, 0.3f) * scale
        ));
        GetTransform()
            .SetScale(Vector3(scale, scale, scale))
            .SetPosition(Vector3(30, 10, 40));

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

        stateMachine = createStateMachine(this, nav, rng);
    }

    void WanderState::Update(float dt)
    {
        if (path.empty()) {
            Debug::DrawLine(owner->GetTransform().GetPosition(), target, Vector4(1, 0, 0, 1));
            navMap->FindPath(owner->GetTransform().GetPosition(), target, path);
        }

        // TODO: Properly navigate the path
        Vector3 waypoint;
        if (path.PopWaypoint(waypoint)) {
            owner->GetTransform().SetPosition(waypoint);
        }
    }
}
