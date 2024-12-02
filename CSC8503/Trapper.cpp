#include "Trapper.h"

#include "AABBVolume.h"
#include "RenderObject.h"

namespace NCL::CSC8503 {
    Trapper::Trapper(
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
            .SetPosition(Vector3(10, 10, 0));

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
    }
}
