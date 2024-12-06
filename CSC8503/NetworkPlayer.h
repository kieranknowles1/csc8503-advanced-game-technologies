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
		const constexpr static float JumpRayLength = 10.0f;
		const constexpr static float JumpForce = 1000.0f;

		bool canJump();
		// Time until the player can jump again
		float jumpCooldown = 0;
	};
}
