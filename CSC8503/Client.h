#pragma once

#include "GameClient.h"

namespace NCL::CSC8503 {
    class Client : PacketReceiver {
    public:
        Client();

        GameClient* getClient() const {
            return client;
        }

        void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source = -1) override;
    private:
        GameClient* client;
    };
}
