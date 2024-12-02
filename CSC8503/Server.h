#pragma once

#include "GameServer.h"

#include "NetworkPlayer.h"

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
    private:
        NetworkedGame* game;
        GameServer* server;

        int packetsToSnapshot = 0;
        // Send a snapshot every n ticks
        int snapshotFrequency = 5;

        void processPlayerConnect(int source);
        void processPlayerDisconnect(int source);
        void processPacket(ClientPacket* packet, int source);

        void broadcastSnapshot(bool deltaFrame);
    };
}
