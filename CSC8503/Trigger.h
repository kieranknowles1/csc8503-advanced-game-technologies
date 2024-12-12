#pragma once

#include <functional>

#include "AABBVolume.h"
#include "GameObject.h"
#include "Debug.h"

namespace NCL::CSC8503 {
    // Basic trigger that executes a function passed to it
    class Trigger : public GameObject {
    public:
        using Func = std::function<void(GameObject*)>;
        Trigger(Func onEnter, Func onExit = nullptr)
            : onEnter(onEnter)
            , onExit(onExit) {
            SetTrigger(true);
            setLayer(LayerMask::Index::Trigger);
        }

        void OnCollisionBegin(GameObject* otherObject) override {
            if (onEnter) {
                onEnter(otherObject);
            }
        }

        void OnCollisionEnd(GameObject* otherObject) override {
            if (onExit) {
                onExit(otherObject);
            }
        }

        void OnUpdate(float dt) override {
            if (boundingVolume && boundingVolume->type == VolumeType::AABB) {
                auto volume = (AABBVolume*)boundingVolume;
                Debug::DrawAABB(GetTransform().GetPosition(), volume->GetHalfDimensions());
            }
        }
    private:
        Func onEnter;
        Func onExit;
    };
}
