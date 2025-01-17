#include "GameClient.h"
#include "./enet/enet.h"
using namespace NCL;
using namespace CSC8503;

GameClient::GameClient()	{
	netHandle = enet_host_create(nullptr, 1, 1, 0, 0);
}

GameClient::~GameClient()	{

}

bool GameClient::Connect(uint32_t addr, int portNum) {
	ENetAddress address;
	address.port = portNum;
	address.host = addr;

	std::cout << "Connecting to server on " << (int)(address.host & 0xff) << "." << (int)((address.host >> 8) & 0xff) << "." << (int)((address.host >> 16) & 0xff) << "." << (int)((address.host >> 24) & 0xff) << ":" << address.port << std::endl;

	netPeer = enet_host_connect(netHandle, &address, 2, 0);

	if (netPeer == nullptr) {
		std::cerr << "Connection to server failed!" << std::endl;
		return false;
	}

	// TODO: Timeout parameter
	ENetEvent event;
	if (enet_host_service(netHandle, &event, 1000) > 0) {
		if (event.type == ENET_EVENT_TYPE_CONNECT) {
			std::cout << "Server connection succeeded!" << std::endl;
			connected = true;
			GamePacket p(GamePacket::Type::Client_ClientConnect);
			ProcessPacket(&p);
			return true;
		}
		else {
			std::cerr << "Unexpected connection status: " << event.type << std::endl;
		}
	}

	return false;
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
		case ENET_EVENT_TYPE_DISCONNECT: {
			std::cout << "Server disconnected!" << std::endl;
			connected = false;
			GamePacket p(GamePacket::Type::Client_ClientDisconnect);
			ProcessPacket(&p);
			break;
		} case ENET_EVENT_TYPE_RECEIVE: {
			ProcessPackedPackets(std::span(event.packet->data, event.packet->dataLength));
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
