#include "NetworkWorld.h"

namespace NCL::CSC8503 {
    NetworkWorld::NetworkWorld(GameClient* client, GameServer* server)
        : client(client), server(server) {
        if (client) {
            client->RegisterPacketHandler(GamePacket::Type::Delta_State, this);
            client->RegisterPacketHandler(GamePacket::Type::Full_State, this);
        }
        reset();
    }

    void NetworkWorld::reset() {
        networkObjects.clear();
        nextId = 0;
    }

    NetworkObject* NetworkWorld::trackObject(GameObject* obj) {
        if (nextId >= ManualIdStart) {
            // We have 2 billion IDs to play with, this would require multi-terrabit
            // network connections to even track
            throw std::runtime_error("Too many network objects. You're doing it wrong.");
		}

        NetworkObject* netObj = new NetworkObject(*obj, nextId);
        obj->SetNetworkObject(netObj);
        networkObjects.emplace(nextId, obj);
        nextId++;
        return netObj;
    }

    NetworkObject* NetworkWorld::trackObjectManual(GameObject* obj, NetworkObject::Id id)
    {
        if (id < ManualIdStart) {
			throw std::runtime_error("Manual ID must be greater than " + std::to_string(ManualIdStart));
		}
        if (networkObjects.find(id) != networkObjects.end()) {
            throw std::runtime_error("Network object with ID " + std::to_string(id) + " already exists");
        }

        NetworkObject* netObj = new NetworkObject(*obj, id);
		obj->SetNetworkObject(netObj);
		networkObjects.emplace(id, obj);
		return netObj;
    }

    GameObject* NetworkWorld::getTrackedObject(NetworkObject::Id id) {
        auto i = networkObjects.find(id);
        // TODO: Possible use-after-free if the GameObject is deleted
        if (i == networkObjects.end()) {
            return nullptr;
        }
        return i->second;
    }

    void NetworkWorld::ProcessPacket(DeltaPacket* payload, int source) {
        GameObject* obj = getTrackedObject(payload->objectID);
        if (obj) {
            obj->GetNetworkObject()->ReadPacket(*payload);
        }
    }

    void NetworkWorld::ProcessPacket(FullPacket* payload, int source) {
        GameObject* obj = getTrackedObject(payload->objectID);
        if (obj) {
            obj->GetNetworkObject()->ReadPacket(*payload);
        }
    }

    void NetworkWorld::ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) {
        switch (type) {
        case GamePacket::Type::Delta_State:
            return ProcessPacket((DeltaPacket*)payload, source);
        case GamePacket::Type::Full_State:
            return ProcessPacket((FullPacket*)payload, source);
        default:
            break;
        }
    }
}
