#include "GameServer.h"
#include "GameWorld.h"
#include "./enet/enet.h"
using namespace NCL;
using namespace CSC8503;

GameServer::GameServer(int onPort, int maxClients)	{
	port		= onPort;
	clientMax	= maxClients;
	clientCount = 0;
	netHandle	= nullptr;
	Initialise();
}

GameServer::~GameServer()	{
	Shutdown();
}

void GameServer::Shutdown() {
	SendGlobalPacket(GamePacket::Type::Shutdown);
	enet_host_destroy(netHandle);
	netHandle = nullptr;
}

bool GameServer::Initialise() {
	ENetAddress address;
	// Create a host that listens to any address on the specified port
	address.host = ENET_HOST_ANY;
	address.port = port;
	netHandle = enet_host_create(&address, clientMax, 1, 0, 0);

	if (!netHandle) {
		std::cerr << __FUNCTION__ << " failed to create network handle!" << std::endl;
		return false;
	}
	return true;
}

// Send a simple event to all clients
bool GameServer::SendGlobalPacket(GamePacket::Type msgID) {
	GamePacket packet(msgID);
	return SendGlobalPacket(packet);
}

// Send a packet with a payload to all clients
bool GameServer::SendGlobalPacket(GamePacket& packet) {
	ENetPacket* enetPacket = enet_packet_create(&packet, packet.GetTotalSize(), 0);
	enet_host_broadcast(netHandle, 0, enetPacket);
	return true;
}

// Process incoming packets from clients
void GameServer::UpdateServer() {
	if (!netHandle) {
		return;
	}

	ENetEvent event;
	while (enet_host_service(netHandle, &event, 0) > 0) {
		int type = event.type;
		int peer = event.peer->incomingPeerID;

		switch (type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			std::cout << "Server: New client connected" << std::endl;
			clientCount++;
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			std::cout << "Server: Client disconnected" << std::endl;
			clientCount--;
			break;
		case ENET_EVENT_TYPE_RECEIVE: {
			std::cout << "Server: Packet received" << std::endl;
			GamePacket* packet = (GamePacket*)event.packet->data;
			ProcessPacket(packet, peer);
			break;
		} default:
			std::cerr << "Server: Unhandled network event type: " << type << std::endl;
		}
		enet_packet_destroy(event.packet);
	}
}

void GameServer::SetGameWorld(GameWorld &g) {
	gameWorld = &g;
}