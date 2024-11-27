#pragma once
#include "TutorialGame.h"
#include "NetworkBase.h"
#include "NetworkObject.h"

namespace NCL {
	namespace CSC8503 {
		class GameServer;
		class GameClient;
		class NetworkPlayer;

		class NetworkedGame : public TutorialGame, public PacketReceiver {
		public:
			NetworkedGame();
			~NetworkedGame();

			void StartAsServer();
			void StartAsClient(char a, char b, char c, char d);

			void UpdateGame(float dt) override;

			void SpawnPlayer();

			void StartLevel();

			void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) override;

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

		protected:
			void HandlePacket(DeltaPacket* payload, int source);
			void HandlePacket(FullPacket* payload, int source);

			void UpdateAsServer(float dt);
			void UpdateAsClient(float dt);

			void BroadcastSnapshot(bool deltaFrame);
			void UpdateMinimumState();
			std::map<int, int> stateIDs;

			// TODO: Make this a Server class
			GameServer* thisServer;

			NetworkObject::Id nextNetworkId;
			// Assign a network object to track a game object
			// Must be called in the same order on all clients and servers
			NetworkObject* createNetworkObject(GameObject* obj);
			GameObject* getNetworkObject(NetworkObject::Id id);
			std::map<NetworkObject::Id, GameObject*> networkObjects;


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

