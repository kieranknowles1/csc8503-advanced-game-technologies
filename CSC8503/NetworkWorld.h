#pragma once

#include <map>

#include "GameObject.h"
#include "NetworkObject.h"
#include "GameClient.h"
#include "GameServer.h"

namespace NCL::CSC8503 {
    // The state of the game over the network, synchronises held objects
    class NetworkWorld : public PacketReceiver {
    public:
        // Reserve IDs below this for level loading
        // IDs above this can be assigned manually
        static const constexpr NetworkObject::Id ManualIdStart = NetworkObject::MaxId >> 1;

        NetworkWorld(GameClient* client, GameServer* server);

        // Reset the network world
        void reset();

        // Start tracking a GameObject over the network
        // This MUST be called in the same order on all clients and servers
        NetworkObject* trackObject(GameObject* obj);

        // Start tracking a GameObject over the network with a specific ID
        // This may be called in any order, but the ID must be unique and
        // represent the same object on all clients and servers
        NetworkObject* trackObjectManual(GameObject* obj, NetworkObject::Id id);

        // Get the GameObject associated with a network ID
        GameObject* getTrackedObject(NetworkObject::Id id);

        void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) override;

        void removeObject(GameObject* obj) {
			networkObjects.erase(obj->GetNetworkObject()->getId());
		}
    private:
        void ProcessPacket(DeltaPacket* payload, int source);
        void ProcessPacket(FullPacket* payload, int source);

        NetworkObject::Id nextId = 0;
        std::map<NetworkObject::Id, GameObject*> networkObjects;

        GameClient* client;
        GameServer* server;
    };

}
