#include "Server.h"

#include "NetworkedGame.h"

namespace NCL::CSC8503 {
    Server::Server(NetworkedGame *game, int maxPlayers)
    {
        this->game = game;
        server = new GameServer(NetworkBase::GetDefaultPort(), maxPlayers);

        server->RegisterPacketHandler(GamePacket::Type::Server_ClientConnect, this);
        server->RegisterPacketHandler(GamePacket::Type::Server_ClientDisconnect, this);
        server->RegisterPacketHandler(GamePacket::Type::ClientState, this);
    }

    void Server::ReceivePacket(GamePacket::Type type, GamePacket *payload, int source)
    {
        switch (type) {
            case GamePacket::Type::Server_ClientConnect:
                return processPlayerConnect(source);
            case GamePacket::Type::Server_ClientDisconnect:
                return processPlayerDisconnect(source);
            case GamePacket::Type::ClientState:
                return processPacket((ClientPacket*)payload, source);
        }
    }

    void Server::processPlayerConnect(int source)
    {
        std::cout << "Player " << source << " has connected!" << std::endl;
        auto playerObj = game->insertPlayer(source);

        PlayerConnectedPacket newPacket(source, playerObj);
        server->SendGlobalPacket(newPacket);

        PlayerListPacket listPacket(game->GetAllPlayers());
        server->SendGlobalPacket(listPacket);

        HelloPacket helloPacket{
            PlayerState{source, playerObj->GetNetworkObject()->getId()}
        };
        server->SendClientPacket(source, helloPacket);
    }

    void Server::processPlayerDisconnect(int source)
    {
        std::cout << "Player " << source << " has disconnected!" << std::endl;
        // TODO: Implement
    }

    void Server::processPacket(ClientPacket *packet, int source)
    {
        auto it = game->GetAllPlayers().find(source);
        if (it != game->GetAllPlayers().end()) {
			it->second.player->setLastInput(packet->input);
		}
    }
}
