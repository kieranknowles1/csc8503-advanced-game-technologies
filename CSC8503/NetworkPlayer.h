#pragma once
#include "GameObject.h"
#include "GameClient.h"

namespace NCL::CSC8503 {
	class NetworkedGame;

	struct PlayerInput {
		bool forward;
		bool backward;
		bool left;
		bool right;
		bool jump;
		bool action;
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
	private:
		int clientId;
		PlayerInput lastInput;

		const constexpr static float JumpCooldown = 0.2f;
		const constexpr static float JumpRayLength = 3.0f;
		// Impulse in kg seconds
		const constexpr static float JumpImpulse = 50.0f;
		// Apply the force over this many ticks
		const constexpr static int JumpTicks = 5;
		// Nudge our position by this at the start of a jump
		// Applied to keep us from immediately colliding, cancelling out the jump
		const constexpr static Vector3 JumpNudge = Vector3(0, 0.25, 0);

		bool canJump();
		int jumpTicksRemaining = 0;
		// Time until the player can jump again
		float jumpCooldown = 0;
	};
}
