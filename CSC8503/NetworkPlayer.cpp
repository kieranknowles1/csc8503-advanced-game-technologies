#include "NetworkPlayer.h"

#include "PhysicsObject.h"

#include "NetworkedGame.h"

namespace NCL::CSC8503 {
	PlayerInput NetworkPlayer::processInput() {
		auto keyboard = Window::GetKeyboard();

		PlayerInput input;
		input.forward  = keyboard->KeyDown(KeyCodes::W);
		input.backward = keyboard->KeyDown(KeyCodes::S);
		input.left     = keyboard->KeyDown(KeyCodes::A);
		input.right    = keyboard->KeyDown(KeyCodes::D);
		input.jump     = keyboard->KeyDown(KeyCodes::SPACE);
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

		if (lastInput.jump) {
			// TODO: Check if we're on the ground
			// no hovercats allowed
			GetPhysicsObject()->AddForce({ 0, force * 10, 0 });
		}
	}
}
