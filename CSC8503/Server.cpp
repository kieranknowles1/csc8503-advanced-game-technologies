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

    void Server::update(float dt)
    {
        if (Window::GetKeyboard()->KeyDown(KeyCodes::F11)) {
            auto reset = GamePacket(GamePacket::Type::Reset);
            server->SendGlobalPacket(reset);
            game->StartLevel();
        }

        packetsToSnapshot--;
        bool delta = packetsToSnapshot >= 0;
        if (!delta) {
			packetsToSnapshot = snapshotFrequency;
		}
        broadcastSnapshot(delta);

        // Process any packets received and flush the send buffer
		server->UpdateServer();
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
    void Server::broadcastSnapshot(bool deltaFrame)
    {
        std::vector<GameObject*>::const_iterator first;
        std::vector<GameObject*>::const_iterator last;

        game->getWorld()->GetObjectIterators(first, last);

        for (auto i = first; i != last; ++i) {
            NetworkObject* o = (*i)->GetNetworkObject();
            if (!o) {
                continue;
            }
            //TODO - you'll need some way of determining
            //when a player has sent the server an acknowledgement
            //and store the lastID somewhere. A map between player
            //and an int could work, or it could be part of a
            //NetworkPlayer struct.

            // TODO: Set to the last state that all players have acknowledged
            int playerState = o->GetLastFullState().stateID;
            GamePacket* newPacket = nullptr;
            if (o->WritePacket(&newPacket, deltaFrame, playerState)) {
                server->SendGlobalPacket(*newPacket);
                delete newPacket;
            }
        }
    }
}
