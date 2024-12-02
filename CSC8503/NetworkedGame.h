#pragma once

#include <map>

#include "Client.h"
#include "Server.h"

#include "Cli.h"
#include "NavigationGrid.h"
#include "TutorialGame.h"
#include "NetworkBase.h"
#include "NetworkObject.h"
#include "NetworkWorld.h"

#include "Rng.h"

namespace NCL {
	namespace CSC8503 {
		static const constexpr int MaxPlayers = 64;

		struct PlayerState {
			int id;
			NetworkObject::Id netObjectID;
		};
		struct LocalPlayerState {
			PlayerState netState;
			NetworkPlayer* player;
		};

		struct PlayerListPacket : public GamePacket {
			char count;
			PlayerState playerStates[MaxPlayers];

			PlayerListPacket(const std::map<int, LocalPlayerState>& players) : GamePacket(Type::PlayerList) {
				size = sizeof(PlayerListPacket) - sizeof(GamePacket);
				count = (char)players.size();

				int i = 0;
				for (auto& player : players) {
					playerStates[i] = player.second.netState;
					i++;
				}
			}
		};

		struct PlayerConnectedPacket : public GamePacket {
			int playerID;
			NetworkObject::Id playerObjectID;

			PlayerConnectedPacket(int id, GameObject* playerObject) : GamePacket(Type::PlayerConnected) {
				size = sizeof(PlayerConnectedPacket) - sizeof(GamePacket);
				playerID = id;
				playerObjectID = playerObject->GetNetworkObject()->getId();
			}
		};

		struct PlayerDisconnectedPacket : public GamePacket {
			int playerID;

			PlayerDisconnectedPacket(int id) : GamePacket(Type::PlayerDisconnected) {
				size = sizeof(PlayerDisconnectedPacket) - sizeof(GamePacket);
				playerID = id;
			}
		};

		struct HelloPacket : public GamePacket {
			PlayerState whoAmI;
			HelloPacket(PlayerState state) : GamePacket(Type::Hello) {
				size = sizeof(HelloPacket) - sizeof(GamePacket);
				whoAmI = state;
			}
		};


		class GameServer;
		class GameClient;
		class NetworkPlayer;

		class NetworkedGame : public TutorialGame, public PacketReceiver {
		public:
			NetworkedGame(const Cli& cli);
			~NetworkedGame();

			void StartAsClient(uint32_t addr);

			void UpdateGame(float dt) override;

			const static constexpr int PlayerIdStart = 100000;

			NetworkPlayer* insertPlayer(int index);

			NetworkPlayer* SpawnPlayer(int id);
			void SpawnMissingPlayers();

			void ClearWorld() override;
			void StartLevel();

			void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) override;

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

			const std::map<int, LocalPlayerState>& GetAllPlayers() const {
				return allPlayers;
			}

		protected:
			void ProcessInput(float dt);

			//void ProcessPacket(PlayerConnectedPacket* payload);
			void ProcessPacket(PlayerDisconnectedPacket* payload);
			void ProcessPacket(PlayerListPacket* payload);
			void ProcessPacket(HelloPacket* payload);

			void UpdateAsClient(float dt);

			void BroadcastSnapshot(bool deltaFrame);
			void UpdateMinimumState();
			std::map<int, int> stateIDs;

			Client* client;
			Server* server;
			NetworkWorld* networkWorld;

			// TODO: Make this a Server class
			// Tick every n seconds
			float inverseTickRate = 1.0f / 60.0f;


			// TODO: Make this a Client class
			GameClient* thisClient;
			// How long since we first tried to connect
			float connectionLength;

			float timeToNextPacket;

			int inputIndex = 0;

			std::map<int, LocalPlayerState> allPlayers;
			PlayerState localPlayer;

			bool freeCam = false;

			// World stuff
			NavigationGrid* maze;

			// Only the server has authority to run RNG
			// TODO: Make this part of the server class
			Rng rng;
		};
	}
}

