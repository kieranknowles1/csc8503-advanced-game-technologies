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

namespace NCL {
	namespace CSC8503 {
		static const constexpr int MaxPlayers = 64;
		static const constexpr int MaxNameLength = 32;

		struct PlayerState {
			int id; // From enet_peer->peerId
			int score = 0;
			NetworkObject::Id netObjectID;
			Vector4 colour;
			unsigned char nameLength;
			char name[MaxNameLength]; // Length prefixed string

			PlayerState(int playerId, NetworkPlayer* player);
			PlayerState() {}

			// Bounds checked getter for name
			std::string_view getName() const;
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

		// Sent by the server to a client to tell them who they are
		// Includes the ID of the object they should control
		// Separate from PlayerConnectedPacket as the client doesn't know who they
		// are. Relying on the first PlayerConnectedPacket would introduce a race condition
		// where if two clients connect at the same time, they could both think they are the same player
		struct HelloPacket : public GamePacket {
			PlayerState whoAmI;
			HelloPacket(PlayerState state) : GamePacket(Type::Hello) {
				size = sizeof(HelloPacket) - sizeof(GamePacket);
				whoAmI = state;
			}
		};

		// Sent by the server to all clients to destroy an object
		struct DestroyPacket : public GamePacket {
			NetworkObject::Id id;
			DestroyPacket(NetworkObject::Id id) : GamePacket(Type::ObjectDestroy) {
				size = sizeof(DestroyPacket) - sizeof(GamePacket);
				this->id = id;
			}
		};


		class GameServer;
		class GameClient;
		class NetworkPlayer;

		class NetworkedGame : public TutorialGame, public PacketReceiver {
		public:
			NetworkedGame(const Cli& cli);
			~NetworkedGame();

			bool isServer() { return server != nullptr; }
			Server* getServer() { return server; }
			Client* getClient() { return client; }

			void StartAsClient(uint32_t addr);

			void UpdateGame(float dt) override;

			const static constexpr int PlayerIdStart = NetworkWorld::ManualIdStart + 1000;

			NetworkPlayer* insertPlayer(int index);

			NetworkPlayer* SpawnPlayer(int clientId, NetworkObject::Id networkId);
			// Spawn player objects for all clients that don't have one
			void SpawnMissingPlayers();

			void ClearWorld() override;
			void StartLevel();

			void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) override;

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

			const std::map<int, LocalPlayerState>& GetAllPlayers() const {
				return allPlayers;
			}

			int getPlayerScore(int id) {
				auto player = allPlayers.find(id);
				if (player != allPlayers.end()) {
					return player->second.netState.score;
				}
				return 0;
			}
			void setPlayerScore(int id, int score) {
				auto player = allPlayers.find(id);
				if (player != allPlayers.end()) {
					player->second.netState.score = score;
				}
			}

			// Remove an object from the game world
			// Delayed until the end of the frame
			void removeObject(GameObject* obj);
		protected:
			void ProcessInput(float dt);

			//void ProcessPacket(PlayerConnectedPacket* payload);
			void ProcessPacket(PlayerDisconnectedPacket* payload);
			void ProcessPacket(PlayerListPacket* payload);
			void ProcessPacket(HelloPacket* payload);
			void ProcessPacket(DestroyPacket* payload);

			void UpdateAsClient(float dt);

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
			const static constexpr int InvalidPlayerId = -2;
			const static constexpr int HostPlayerId = -1;
			int localPlayerId = InvalidPlayerId;

			bool freeCam = false;

			// World stuff
			NavigationGrid* maze;

			// Objects to be deleted at the end of the frame
			std::vector<GameObject*> graveyard;
			void clearGraveyard();
		};
	}
}

