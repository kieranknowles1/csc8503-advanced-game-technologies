#pragma once

#include "GameServer.h"

#include "NetworkPlayer.h"
#include "NetworkObject.h"

namespace NCL::CSC8503 {
    class NetworkedGame;
    struct ClientHelloPacket;

    class Server : PacketReceiver {
    public:
        Server(NetworkedGame* game, int maxPlayers);

        GameServer* getServer() const {
            return server;
        }

        void update(float dt);

        void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source = -1) override;

        void sendPlayerList();

        void broadcastObjectDestroy(NetworkObject::Id id);

        // Force a full sync of all objects next frame
        // This is useful when a new player joins
        void forceFullSync() {
			forceFullBroadcast = true;
		}
    private:
        bool forceFullBroadcast = false;
        NetworkedGame* game;
        GameServer* server;

        void processPlayerDisconnect(int source);
        void processPacket(ClientPacket* packet, int source);
        void processPacket(ClientHelloPacket* packet, int source);

        void broadcastDeltas();
    };
}
