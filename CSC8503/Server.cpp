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

        broadcastDeltas();

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

    void Server::sendPlayerList()
    {
        PlayerListPacket listPacket(game->GetAllPlayers());
        server->SendGlobalPacket(listPacket);
    }

    void Server::broadcastObjectDestroy(NetworkObject::Id id)
    {
        DestroyPacket destroyPacket(id);
		server->SendGlobalPacket(destroyPacket);
    }

    void Server::processPlayerConnect(int source)
    {
        std::cout << "Player " << source << " has connected!" << std::endl;
        auto netState = game->generateNetworkState(source);

        sendPlayerList();

        HelloPacket helloPacket{
            netState
        };
        server->SendClientPacket(source, helloPacket);

        game->GetAllPlayers().emplace(source, LocalPlayerState{netState});
        game->SpawnMissingPlayers();
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
    void Server::broadcastDeltas()
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

            const constexpr int SendDeltaThreshold = 50;
            const constexpr int SendFullThreshold = 1000;

            // TODO: Set to the last state that all players have acknowledged
            int playerState = o->GetLastFullState().stateID;
            GamePacket* newPacket = nullptr;

            // Get an approximation of how much the state has changed since
            // the last packet of each type was sent
            // These values are very approximate and could be tuned if needed
            int fromLastDelta = o->getDeltaError(o->GetLastDeltaState());
            int fromLastFull = o->getDeltaError(o->GetLastFullState());

            // If we've moved too far from the last full, send a new full
            bool sendFull = fromLastFull > SendFullThreshold;
            // Don't send anything if the delta is still good enough
            if (fromLastDelta > SendDeltaThreshold || sendFull) {
                GamePacket* newPacket = nullptr;
                if (o->WritePacket(&newPacket, !sendFull, playerState)) {
                    server->SendGlobalPacket(*newPacket);
                    delete newPacket;
                }
            }
        }
    }
}
