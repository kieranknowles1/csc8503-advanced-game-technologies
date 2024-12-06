#pragma once

#include "GameServer.h"

#include "NetworkPlayer.h"
#include "NetworkObject.h"

namespace NCL::CSC8503 {
    class NetworkedGame;

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
    private:
        NetworkedGame* game;
        GameServer* server;

        void processPlayerConnect(int source);
        void processPlayerDisconnect(int source);
        void processPacket(ClientPacket* packet, int source);

        void broadcastDeltas();
    };
}
