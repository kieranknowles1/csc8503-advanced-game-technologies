#pragma once

#include <cfloat>

#include "LayerMask.h"

namespace NCL {
	namespace Maths {
		struct RayCollision {
			void*		node;			//Node that was hit
			Vector3		collidedAt;		//WORLD SPACE position of the collision!
			float		rayDistance;

			// Match all layers by default
			RayCollision(void*node, Vector3 collidedAt, CSC8503::LayerMask mask = CSC8503::LayerMask()) {
				this->node			= node;
				this->collidedAt	= collidedAt;
				this->rayDistance	= 0.0f;
			}

			RayCollision() {
				node			= nullptr;
				rayDistance		= FLT_MAX;
			}
		};

		class Ray {
		public:
			Ray(Vector3 position, Vector3 direction) {
				this->position  = position;
				this->direction = direction;
			}
			~Ray(void) {}

			Vector3 GetPosition() const {return position;	}

			Vector3 GetDirection() const {return direction;	}

			CSC8503::LayerMask getMask() const { return mask; }
			void setMask(CSC8503::LayerMask m) { mask = m; }

		protected:
			Vector3 position;	//World space position
			Vector3 direction;	//Normalised world space direction
			// Only collide with objects on these layers
			CSC8503::LayerMask mask;
		};
	}
}
