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

		// TODO: Move this to NetworkBase.h
		// A fixed size, length prefixed string. Suitable for network packets
		template <int MaxLength>
		struct SizedString {
			using SizeType = unsigned char;
			static_assert(MaxLength < std::numeric_limits<SizeType>::max(), "Max length too large for size type");
			SizeType length;
			char data[MaxLength];

			std::string_view get() const {
				if (length > MaxLength) {
					throw std::runtime_error("String too long for packet data!");
				}
				return std::string_view(data, length);
			};
			void set(std::string_view str) {
				if (str.length() > MaxLength) {
					throw std::runtime_error("String too long for packet data!");
				}
				length = (SizeType)str.length();
				memset(data, 0, MaxLength);
				memcpy(data, str.data(), str.length());
			}
		};

		struct PlayerState {
			int id; // From enet_peer->peerId
			int score = 0;
			NetworkObject::Id netObjectID;
			Vector4 colour;
			SizedString<MaxNameLength> name;
		};
		struct LocalPlayerState {
			PlayerState netState;
			NetworkPlayer* player;

			LocalPlayerState(PlayerState netState) : netState(netState), player(nullptr) {}
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
		struct ServerHelloPacket : public GamePacket {
			PlayerState whoAmI;
			ServerHelloPacket(PlayerState state) : GamePacket(Type::ServerHello) {
				size = sizeof(ServerHelloPacket) - sizeof(GamePacket);
				whoAmI = state;
			}
		};

		struct ClientHelloPacket : public GamePacket {
			SizedString<MaxNameLength> name;
			ClientHelloPacket() : GamePacket(Type::ClientHello) {
				size = sizeof(ClientHelloPacket) - sizeof(GamePacket);
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

			void StartAsClient(uint32_t addr, std::string_view name);

			void UpdateGame(float dt) override;

			const static constexpr int PlayerIdStart = NetworkWorld::ManualIdStart + 1000;

			NetworkPlayer* SpawnPlayer(PlayerState state);
			// Spawn player objects for all clients that don't have one
			void SpawnMissingPlayers();

			void ClearWorld() override;

			GameObject* AddNetworkCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, bool axisAligned = false);
			void AddBridgeToWorld();

			void StartLevel();

			void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) override;

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

			std::map<int, LocalPlayerState>& GetAllPlayers() {
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

			PlayerState generateNetworkState(int clientId, std::string_view name);
		protected:
			void ProcessInput(float dt);

			//void ProcessPacket(PlayerConnectedPacket* payload);
			void ProcessPacket(PlayerDisconnectedPacket* payload);
			void ProcessPacket(PlayerListPacket* payload);
			void ProcessPacket(ServerHelloPacket* payload);
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

