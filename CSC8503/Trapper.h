#pragma once

#include "StateMachine.h"
#include "Mesh.h"
#include "Shader.h"
#include "NavigationGrid.h"
#include "GameObject.h"

namespace NCL::CSC8503 {
    class Trapper : public GameObject {
        public:
            Trapper(Rendering::Mesh* mesh, Rendering::Shader* shader, NavigationGrid* nav);

        protected:
            NavigationGrid* navMap;

            StateMachine* stateMachine;
    };
}
