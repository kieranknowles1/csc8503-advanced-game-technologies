#include "GameClient.h"
#include "./enet/enet.h"
using namespace NCL;
using namespace CSC8503;

GameClient::GameClient()	{
	netHandle = enet_host_create(nullptr, 1, 1, 0, 0);
}

GameClient::~GameClient()	{
	
}

bool GameClient::Connect(uint8_t a, uint8_t b, uint8_t c, uint8_t d, int portNum) {
	ENetAddress address;
	address.port = portNum;
	address.host = (d << 24) | (c << 16) | (b << 8) | a;
	netPeer = enet_host_connect(netHandle, &address, 2, 0);

	if (netPeer == nullptr) {
		std::cerr << "Connection to server failed!" << std::endl;
		return false;
	}
	return true;
}

void GameClient::UpdateClient() {
	// eNet generates a queue of packets - we need to pop all of these and process them
	if (netHandle == nullptr) {
		return;
	}

	// Loop while we have work to do
	ENetEvent event;
	while (enet_host_service(netHandle, &event, 0) > 0) {
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			std::cout << "Server connection succeeded!" << std::endl;
			connected = true;
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			std::cout << "Server disconnected!" << std::endl;
			connected = false;
			break;
		case ENET_EVENT_TYPE_RECEIVE: {
			std::cout << "Packet received!" << std::endl;
			GamePacket* payload = (GamePacket*)event.packet->data;
			ProcessPacket(payload);
			break;
		} default:
			std::cerr << "Unhandled network event type: " << event.type << std::endl;
			break;
		}
		// Free the packet now that we've processed it
		enet_packet_destroy(event.packet);
	}
}

void GameClient::SendPacket(GamePacket&  payload) {
	ENetPacket* packet = enet_packet_create(&payload, payload.GetTotalSize(), 0);
	enet_peer_send(netPeer, 0, packet);
}
