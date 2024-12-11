#include "NetworkPlayer.h"

#include "PhysicsObject.h"

#include "NetworkedGame.h"
#include "Trapper.h"

namespace NCL::CSC8503 {
	NetworkPlayer::NetworkPlayer(int clientId) : clientId(clientId) {
		setLayer(LayerMask::Index::Actor);
	}

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

	void NetworkPlayer::handleJumpInput(float dt) {
		auto [canJump, collision] = this->canJump();
		if (!canJump) {
			return;
		}

		jumpCooldown = JumpCooldown;
		// Nudge us to avoid an immediate collision
		GetTransform().SetPosition(GetTransform().GetPosition() + JumpNudge);

		// All forces last one tick, so apply a large force for one tick to simulate an impulse
		float force = impulseToForce(JumpImpulse, dt);
		
		GetPhysicsObject()->AddForce({ 0, force, 0 });
		// Newton's third law: for every action there is an equal and opposite reaction
		// Apply an impulse to the object we jumped off of
		GameObject* other = (GameObject*)collision.node;
		PhysicsObject* otherPhysics = other ? other->GetPhysicsObject() : nullptr;
		if (otherPhysics) {
			otherPhysics->AddForceAtPosition({ 0, -force, 0 }, collision.collidedAt);
		}
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

		if (lastInput.jump) {
			handleJumpInput(dt);
		}
		lastInput.jump = false;
		jumpCooldown -= dt;

		// Reset the player if they fall off the map
		if (GetTransform().GetPosition().y < -100) {
			Reset();
		}
	}

	void NetworkPlayer::OnCollisionBegin(GameObject* other) {
		if (dynamic_cast<Trapper*>(other)) {
			std::cout << "Player hit a trapper!" << std::endl;
			Reset();
		}
	}

	std::pair<bool, RayCollision> NetworkPlayer::canJump() {
		if (jumpCooldown > 0) {
			return { false, {} };
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
		return { closeEnough, closestCollision };
	}
}
