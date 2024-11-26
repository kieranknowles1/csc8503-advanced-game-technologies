#include "CollisionDetection.h"

#include <cmath>

#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "Window.h"
#include "Maths.h"
#include "Debug.h"

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray&r, const Plane&p, RayCollision& collisions) {
	float ln = Vector::Dot(p.GetNormal(), r.GetDirection());

	if (ln == 0.0f) {
		return false; //direction vectors are perpendicular!
	}

	Vector3 planePoint = p.GetPointOnPlane();

	Vector3 pointDir = planePoint - r.GetPosition();

	float d = Vector::Dot(pointDir, p.GetNormal()) / ln;

	collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d);

	return true;
}

bool CollisionDetection::RayIntersection(const Ray& r,GameObject& object, RayCollision& collision) {
	bool hasCollided = false;

	const Transform& worldTransform = object.GetTransform();
	const CollisionVolume* volume	= object.GetBoundingVolume();

	if (!volume) {
		return false;
	}

	switch (volume->type) {
		case VolumeType::AABB:		hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume&)*volume	, collision); break;
		case VolumeType::OBB:		hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume&)*volume	, collision); break;
		case VolumeType::Sphere:	hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume&)*volume	, collision); break;

		case VolumeType::Capsule:	hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume&)*volume, collision); break;
	}

	return hasCollided;
}

// Check for a collision between a ray and a box
bool CollisionDetection::RayBoxIntersection(const Ray&r, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();

	// Determine which planes to check
	Vector3 tVals(-1, -1, -1);
	// Get the 3 best intersections based on which direction we're coming from
	for (int i = 0; i < 3; i++) {
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}
		// else tVals[i] remains -1 as the ray is parallel to the planes
	}
	// The distance to the first plane we hit
	float bestT = Vector::GetMaxElement(tVals);
	if (bestT < 0.0) {
		return false; // All intersections are behind us
	}

	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001f; // Tolerance in our intersection tests, allows for floating point errors
	for (int i = 0; i < 3; i++) {
		if (intersection[i] + epsilon < boxMin[i] || intersection[i] - epsilon > boxMax[i]) {
			return false; // The best intersection doesn't touch one face of the box, so it's not a hit
		}
	}
	collision.collidedAt = intersection;
	collision.rayDistance = bestT;
	return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray&r, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {
	// Nothing special here, let RayBoxIntersection do the work
	return RayBoxIntersection(r, worldTransform.GetPosition(), volume.GetHalfDimensions(), collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray&r, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();

	// Transform the ray such that it's in the space of an AABB represented by the OBB
	Matrix3 transform = Quaternion::RotationMatrix<Matrix3>(orientation);
	Matrix3 inverseTransform = Quaternion::RotationMatrix<Matrix3>(orientation.Conjugate());

	Vector3 localRayPos = r.GetPosition() - position;
	Ray transformedRay(inverseTransform * localRayPos, inverseTransform * r.GetDirection());
	bool collided = RayBoxIntersection(transformedRay, Vector3(), volume.GetHalfDimensions(), collision);
	if (collided) {
		// Transform the collision point back into world space
		collision.collidedAt = transform * collision.collidedAt + position;
	}
	return collided;
}

bool CollisionDetection::RaySphereIntersection(const Ray&r, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePosition = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();

	Vector3 direction = (spherePosition - r.GetPosition());

	// Project the sphere's origin onto the ray
	// This is the distance along the ray where the sphere's center is closest, if it's less than 0, the sphere is behind the ray
	float sphereProj = Vector::Dot(direction, r.GetDirection());
	if (sphereProj < 0) {
		return false; // Origin is behind the ray, we will never hit the sphere
	}

	// At this closest point, find the distance to the sphere's center
	Vector3 closest = r.GetPosition() + (r.GetDirection() * sphereProj);
	float distance = Vector::Length(closest - spherePosition);
	if (distance > sphereRadius) {
		return false; // Ray is not close enough to hit the sphere
	}

	// Using the distance and pythagoras theorem, find the distance from the plane representing the center of the sphere facing the ray,
	// to the point where the ray hits the sphere
	// Use this to get the 3D point of collision
	float offset = sqrt(
		(sphereRadius * sphereRadius) - (distance * distance)
	);
	collision.rayDistance = sphereProj - offset;
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);
	return true;
}

bool CollisionDetection::RayCapsuleIntersection(const Ray& r, const Transform& worldTransform, const CapsuleVolume& volume, RayCollision& collision) {
	return false; // TODO: Implement
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();

	if (!volA || !volB) {
		return false;
	}

	collisionInfo.a = a;
	collisionInfo.b = b;

	Transform& transformA = a->GetTransform();
	Transform& transformB = b->GetTransform();

	// Use the bitwise OR of the two volume types to determine the type of collision
	// One bit will be set for collision between two of the same type, two bits for different types
	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	//Two AABBs
	if (pairType == VolumeType::AABB) {
		return AABBIntersection((AABBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	//Two Spheres
	if (pairType == VolumeType::Sphere) {
		return SphereIntersection((SphereVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	//Two OBBs
	if (pairType == VolumeType::OBB) {
		return OBBIntersection((OBBVolume&)*volA, transformA, (OBBVolume&)*volB, transformB, collisionInfo);
	}
	//Two Capsules

	//AABB vs Sphere pairs
	// We may need to swap the order of the objects depending on the order of the types
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBSphereIntersection((AABBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	//OBB vs sphere pairs
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Sphere) {
		return OBBSphereIntersection((OBBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBSphereIntersection((OBBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	// OBB vs AABB pairs
	// Handle these as OBB vs OBB, but with one of the OBBs being axis-aligned
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::OBB) {
		OBBVolume fakeVolume(((AABBVolume&)*volA).GetHalfDimensions());
		return OBBIntersection(fakeVolume, transformA, (OBBVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::OBB && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		OBBVolume fakeVolume(((AABBVolume&)*volB).GetHalfDimensions());
		return OBBIntersection(fakeVolume, transformB, (OBBVolume&)*volA, transformA, collisionInfo);
	}

	//Capsule vs other interactions
	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
		return SphereCapsuleIntersection((CapsuleVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return SphereCapsuleIntersection((CapsuleVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::AABB) {
		return AABBCapsuleIntersection((CapsuleVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	if (volB->type == VolumeType::Capsule && volA->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBCapsuleIntersection((CapsuleVolume&)*volB, transformB, (AABBVolume&)*volA, transformA, collisionInfo);
	}

	return false;
}

bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {
	Vector3 delta = posB - posA;
	Vector3 totalSize = halfSizeA + halfSizeB;

	if (std::abs(delta.x) < totalSize.x &&
		std::abs(delta.y) < totalSize.y &&
		std::abs(delta.z) < totalSize.z) {
		return true;
	}
	return false;
}

//AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxAPos = worldTransformA.GetPosition();
	Vector3 boxBPos = worldTransformB.GetPosition();

	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();

	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);
	if (!overlap) {
		return false; // No collision
	}

	// We only apply collisions in a single axis at a time. Use the one that penetrates the most
	static const Vector3 faces[6] = {
		Vector3(-1, 0, 0), Vector3(1, 0, 0),
		Vector3(0, -1, 0), Vector3(0, 1, 0),
		Vector3(0, 0, -1), Vector3(0, 0, 1)
	};
	Vector3 maxA = boxAPos + boxASize;
	Vector3 minA = boxAPos - boxASize;

	Vector3 maxB = boxBPos + boxBSize;
	Vector3 minB = boxBPos - boxBSize;

	float distances[6] = {
		(maxB.x - minA.x), // Box b to the left of box a
		(maxA.x - minB.x), // Box b to the right of box a
		(maxB.y - minA.y), // Box b to bottom of box a
		(maxA.y - minB.y), // Box b to top of box a
		(maxB.z - minA.z), // Box b to far of box a
		(maxA.z - minB.z)  // Box b to near of box a
	};
	float penetration = FLT_MAX;
	Vector3 bestAxis;

	// Get the axis of least penetration
	for (int i = 0; i < 6; i++) {
		if (distances[i] < penetration) {
			penetration = distances[i];
			bestAxis = faces[i];
		}
	}
	// Collisions apply at the origin of both boxes with a normal in a single direction.
	collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
	return true;
}

//Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	float sumRadii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 deltaPosition = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	float deltaLength = Vector::Length(deltaPosition);

	if (deltaLength > sumRadii) {
		return false; // No collision
	}

	float penetration = sumRadii - deltaLength;
	Vector3 normal = Vector::Normalise(deltaPosition);
	Vector3 localA = normal * volumeA.GetRadius();
	Vector3 localB = -normal * volumeB.GetRadius();

	collisionInfo.AddContactPoint(localA, localB, normal, penetration);
	return true;
}

//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxSize = volumeA.GetHalfDimensions();
	Vector3 minusBoxSize = -boxSize;
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

	Vector3 aaBBClosestPoint = Vector::Clamp(delta, minusBoxSize, boxSize);

	// What is the closest point on the box to the sphere?
	Vector3 localPoint = delta - aaBBClosestPoint;
	float distance = Vector::Length(localPoint);

	if (distance > volumeB.GetRadius()) {
		return false; // No collision
	}

	Vector3 collisionNormal = Vector::Normalise(localPoint);
	float penetration = volumeB.GetRadius() - distance;
	Vector3 localA = Vector3(); // This is an AABB, so just use the closest point and don't apply torque
	Vector3 localB = -collisionNormal * volumeB.GetRadius();

	collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
	return true;
}

bool  CollisionDetection::OBBSphereIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	// Transform the sphere into the space of the OBB
	Quaternion orientation = worldTransformA.GetOrientation();
	Vector3 position = worldTransformA.GetPosition();

	Matrix3 transform = Quaternion::RotationMatrix<Matrix3>(orientation);
	Matrix3 inverseTransform = Quaternion::RotationMatrix<Matrix3>(orientation.Conjugate());

	Vector3 localSpherePos = inverseTransform * (worldTransformB.GetPosition() - position);
	// Find the closest point on the OBB to the sphere
	Vector3 halfDimensions = volumeA.GetHalfDimensions();
	Vector3 minusHalfDimensions = -halfDimensions;
	Vector3 closestPoint = Vector::Clamp(localSpherePos, minusHalfDimensions, halfDimensions);

	// Transform the closest point back into world space
	Vector3 closestWorldPoint = transform * closestPoint + position;
	Vector3 delta = worldTransformB.GetPosition() - closestWorldPoint;

	float distance = Vector::Length(delta);
	if (distance > volumeB.GetRadius()) {
		return false; // No collision
	}

	Vector3 collisionNormal = Vector::Normalise(delta);
	float penetration = volumeB.GetRadius() - distance;
	Vector3 localA = closestWorldPoint - position; // This is an OBB, so use the closest point to apply torque
	Vector3 localB = -collisionNormal * volumeB.GetRadius();
	collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
	return true;
}

bool CollisionDetection::AABBCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false; // TODO: Implement
}

bool CollisionDetection::SphereCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false; // TODO: Implement
}

// Based on Haalef's answer from
// https://stackoverflow.com/questions/47866571/simple-oriented-bounding-box-obb-collision-detection-explaining
// Extended to track the axis of least penetration

bool hasSeparatingPlane(Vector3 relativePosition, Vector3 plane, OBBVolume box1, Transform transform1, OBBVolume box2, Transform transform2, float& minPenetration, Vector3& minAxis)
{
	Vector3 b1X = transform1.GetOrientation().Rotate(Vector3(1, 0, 0));
	Vector3 b1Y = transform1.GetOrientation().Rotate(Vector3(0, 1, 0));
	Vector3 b1Z = transform1.GetOrientation().Rotate(Vector3(0, 0, 1));
	Vector3 b2X = transform2.GetOrientation().Rotate(Vector3(1, 0, 0));
	Vector3 b2Y = transform2.GetOrientation().Rotate(Vector3(0, 1, 0));
	Vector3 b2Z = transform2.GetOrientation().Rotate(Vector3(0, 0, 1));

	float penetration = (
		fabs(Vector::Dot(b1X * box1.GetHalfDimensions().x, plane)) +
		fabs(Vector::Dot(b1Y * box1.GetHalfDimensions().y, plane)) +
		fabs(Vector::Dot(b1Z * box1.GetHalfDimensions().z, plane)) +
		fabs(Vector::Dot(b2X * box2.GetHalfDimensions().x, plane)) +
		fabs(Vector::Dot(b2Y * box2.GetHalfDimensions().y, plane)) +
		fabs(Vector::Dot(b2Z * box2.GetHalfDimensions().z, plane))
		) - fabs(Vector::Dot(relativePosition, plane));
	if (penetration > 0) {
		if (penetration < minPenetration) {
			minPenetration = penetration;
			minAxis = plane;
		}
		return false;
	}
	return true;
}

// Very janky, but functional. Touchy = breaky
bool CollisionDetection::OBBIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const OBBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Vector3 posA = worldTransformA.GetPosition();
	Vector3 posB = worldTransformB.GetPosition();
	Vector3 relativePos = posB - posA;

	Vector3 v1AxisX = worldTransformA.GetOrientation().Rotate(Vector3(1, 0, 0));
	Vector3 v1AxisY = worldTransformA.GetOrientation().Rotate(Vector3(0, 1, 0));
	Vector3 v1AxisZ = worldTransformA.GetOrientation().Rotate(Vector3(0, 0, 1));
	Vector3 v2AxisX = worldTransformB.GetOrientation().Rotate(Vector3(1, 0, 0));
	Vector3 v2AxisY = worldTransformB.GetOrientation().Rotate(Vector3(0, 1, 0));
	Vector3 v2AxisZ = worldTransformB.GetOrientation().Rotate(Vector3(0, 0, 1));

	float minPenetration = FLT_MAX;
	Vector3 minAxis;
	bool hasPlane =
		hasSeparatingPlane(relativePos, v1AxisX, volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, v1AxisY, volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, v1AxisZ, volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, v2AxisX, volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, v2AxisY, volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, v2AxisZ, volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis);
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisX, v2AxisX), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisX, v2AxisY), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisX, v2AxisZ), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisY, v2AxisX), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisY, v2AxisY), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisY, v2AxisZ), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisZ, v2AxisX), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisZ, v2AxisY), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis) ||
		hasSeparatingPlane(relativePos, Vector::Cross(v1AxisZ, v2AxisZ), volumeA, worldTransformA, volumeB, worldTransformB, minPenetration, minAxis);
	if (hasPlane) return false;

	Vector3 normal = Vector::Normalise(minAxis);
	Vector3 localA = normal * Vector::Dot(posB - posA, normal);
	Vector3 localB = -normal * Vector::Dot(posB - posA, normal);
	collisionInfo.AddContactPoint(localA, localB, normal, minPenetration);
	return true;
}

Matrix4 GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix::Translation(position) *
		Matrix::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Matrix4 GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	float negDepth = nearPlane - farPlane;

	float invNegDepth = negDepth / (2 * (farPlane * nearPlane));

	Matrix4 m;

	float h = 1.0f / tan(fov*PI_OVER_360);

	m.array[0][0] = aspect / h;
	m.array[1][1] = tan(fov * PI_OVER_360);
	m.array[2][2] = 0.0f;

	m.array[2][3] = invNegDepth;//// +PI_OVER_360;
	m.array[3][2] = -1.0f;
	m.array[3][3] = (0.5f / nearPlane) + (0.5f / farPlane);

	return m;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const PerspectiveCamera& cam) {
	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	float aspect = Window::GetWindow()->GetScreenAspect();
	float fov		= cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane  = cam.GetFarPlane();

	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	Matrix4 proj  = cam.BuildProjectionMatrix(aspect);

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const PerspectiveCamera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2i screenSize	= Window::GetWindow()->GetScreenSize();

	//We remove the y axis mouse position from height as OpenGL is 'upside down',
	//and thinks the bottom left is the origin, instead of the top left!
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	//We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
	//causes the unproject function to go a bit weird.
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c = Vector::Normalise(c);

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov*PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f*(nearPlane*farPlane) / neg_depth;

	m.array[0][0] = aspect / h;
	m.array[1][1] = tan(fov * PI_OVER_360);
	m.array[2][2] = 0.0f;

	m.array[2][3] = 1.0f / d;

	m.array[3][2] = 1.0f / e;
	m.array[3][3] = -c / (d * e);

	return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix::Translation(position) *
		Matrix::Rotation(yaw, Vector3(0, 1, 0)) *
		Matrix::Rotation(pitch, Vector3(1, 0, 0));

	return iview;
}


/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const PerspectiveCamera& c) {
	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());


	Vector2i screenSize = Window::GetWindow()->GetScreenSize();

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

