#pragma once
#include "GameObject.h"
#include "GameClient.h"
#include "Ray.h"

namespace NCL::CSC8503 {
	class NetworkedGame;

	struct PlayerInput {
		bool forward = false;
		bool backward = false;
		bool left = false;
		bool right = false;
		bool jump = false;
		bool action = false;
	};

	struct ClientPacket : public GamePacket {
		// How many packets have been sent
		int		index;
		PlayerInput input;

		ClientPacket() : GamePacket(Type::ClientState) {
			size = sizeof(ClientPacket);
		}
	};

	class NetworkPlayer : public GameObject {
	public:
		NetworkPlayer(int clientId);

		PlayerInput processInput();

		void setLastInput(const PlayerInput& input) {
			lastInput = input;
		}

		void OnUpdate(float dt) override;
		void OnCollisionBegin(GameObject* other) override;

		Tag getTag() const override {
			return Tag::Player;
		}

		int getClientID() const {
			return clientId;
		}

		bool isInHome() const {
			return inHome;
		}
		void setInHome(bool value) {
			inHome = value;
		}
	private:
		void handleJumpInput(float dt);

		int clientId;
		PlayerInput lastInput;

		bool inHome = false;

		const constexpr static float JumpCooldown = 0.2f;
		const constexpr static float JumpRayLength = 3.0f;
		// Impulse in kg seconds
		const constexpr static float JumpImpulse = 50.0f;
		// Nudge our position by this at the start of a jump
		// Applied to keep us from immediately colliding, cancelling out the jump
		const constexpr static Vector3 JumpNudge = Vector3(0, 0.25, 0);

		std::pair<bool, RayCollision> canJump();
		// Time until the player can jump again
		float jumpCooldown = 0;
	};
}
