#pragma once

#include <map>

#include "Cli.h"
#include "TutorialGame.h"
#include "NetworkBase.h"
#include "NetworkObject.h"
#include "NetworkWorld.h"

namespace NCL {
	namespace CSC8503 {
		struct PlayerConnectedPacket : public GamePacket {
			int playerID;

			PlayerConnectedPacket(int id) : GamePacket(Type::PlayerConnected) {
				size = sizeof(PlayerConnectedPacket) - sizeof(GamePacket);
				playerID = id;
			}
		};

		struct PlayerDisconnectedPacket : public GamePacket {
			int playerID;

			PlayerDisconnectedPacket(int id) : GamePacket(Type::PlayerDisconnected) {
				size = sizeof(PlayerDisconnectedPacket) - sizeof(GamePacket);
				playerID = id;
			}
		};


		class GameServer;
		class GameClient;
		class NetworkPlayer;

		class NetworkedGame : public TutorialGame, public PacketReceiver {
		public:
			NetworkedGame(const Cli& cli);
			~NetworkedGame();

			void StartAsServer();
			void StartAsClient(char a, char b, char c, char d);

			void UpdateGame(float dt) override;

			void SpawnPlayer();

			void StartLevel();

			void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) override;

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

		protected:
			void ProcessPacket(PlayerConnectedPacket* payload);
			void ProcessPacket(PlayerDisconnectedPacket* payload);

			void ProcessPlayerConnect(int playerID);
			void ProcessPlayerDisconnect(int playerID);

			void UpdateAsServer(float dt);
			void UpdateAsClient(float dt);

			void BroadcastSnapshot(bool deltaFrame);
			void UpdateMinimumState();
			std::map<int, int> stateIDs;

			// TODO: Make this a Server class
			GameServer* thisServer;

			NetworkWorld* networkWorld;

			// TODO: Make this a Client class
			GameClient* thisClient;
			// How long since we first tried to connect
			float connectionLength;

			float timeToNextPacket;
			int packetsToSnapshot;

			//std::vector<NetworkObject*> networkObjects;

			std::map<int, GameObject*> serverPlayers;
			GameObject* localPlayer;
		};
	}
}

