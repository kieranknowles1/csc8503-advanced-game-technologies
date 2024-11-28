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
		PlayerInput processInput();

		void setLastInput(const PlayerInput& input) {
			lastInput = input;
		}

		void OnUpdate(float dt) override;
	private:
		PlayerInput lastInput;
	};
}
