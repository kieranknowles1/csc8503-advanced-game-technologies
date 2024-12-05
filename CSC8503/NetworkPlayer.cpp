#include "NetworkPlayer.h"

#include "PhysicsObject.h"

#include "NetworkedGame.h"
#include "Trapper.h"

namespace NCL::CSC8503 {
	PlayerInput NetworkPlayer::processInput() {
		// std::cout << transform.GetPosition().x << " " << transform.GetPosition().y << " " << transform.GetPosition().z << std::endl;
		auto keyboard = Window::GetKeyboard();

		PlayerInput input;
		input.forward  = keyboard->KeyDown(KeyCodes::W);
		input.backward = keyboard->KeyDown(KeyCodes::S);
		input.left     = keyboard->KeyDown(KeyCodes::A);
		input.right    = keyboard->KeyDown(KeyCodes::D);
		// Always jump for one frame. |= the input in case processInput is called multiple times in one frame
		input.jump = lastInput.jump || keyboard->KeyPressed(KeyCodes::SPACE);
		input.action   = keyboard->KeyDown(KeyCodes::F);
		return input;
	}

	void NetworkPlayer::OnUpdate(float dt) {
		float force = 1000 * dt;
		Vector3 forwardsForce = GetTransform().GetOrientation() * Vector3(0, 0, force);
		forwardsForce = forwardsForce * 3;

		if (lastInput.forward) {
			GetPhysicsObject()->AddForce(forwardsForce);
		} else if (lastInput.backward) {
			GetPhysicsObject()->AddForce(-forwardsForce);
		}

		if (lastInput.left) {
			GetPhysicsObject()->AddTorque({ 0, force, 0 });
		} else if (lastInput.right) {
			GetPhysicsObject()->AddTorque({ 0, -force, 0 });
		}

		if (lastInput.jump && canJump()) {
			GetPhysicsObject()->AddForce({ 0, JumpForce, 0 });
			jumpCooldown = JumpCooldown;
		}
		lastInput.jump = false;
		jumpCooldown -= dt;
	}

	void NetworkPlayer::OnCollisionBegin(GameObject* other) {
		if (dynamic_cast<Trapper*>(other)) {
			std::cout << "Player hit a trapper!" << std::endl;
			Reset();
		}
	}

	bool NetworkPlayer::canJump() {
		if (jumpCooldown > 0) {
			return false;
		}
		auto world = GetWorld();
		Ray ray(GetTransform().GetPosition(), Vector3(0, -1, 0));
		RayCollision closestCollision;
		bool hit = world->Raycast(ray, closestCollision, true, this);

		bool closeEnough = hit && closestCollision.rayDistance < JumpRayLength;
		Debug::DrawLine(
			ray.GetPosition(),
			ray.GetPosition() + ray.GetDirection() * (closeEnough ? closestCollision.rayDistance : JumpRayLength),
			closeEnough ? Debug::GREEN : Debug::RED,
			5.0f
		);
		return closeEnough;
	}
}
