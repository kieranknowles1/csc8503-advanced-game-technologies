#include "NetworkBase.h"
#include "./enet/enet.h"
NetworkBase::NetworkBase()	{
	netHandle = nullptr;
}

NetworkBase::~NetworkBase()	{
	if (netHandle) {
		enet_host_destroy(netHandle);
	}
}

void NetworkBase::Initialise() {
	enet_initialize();
}

void NetworkBase::Destroy() {
	enet_deinitialize();
}

bool NetworkBase::ProcessPacket(GamePacket* packet, int peerID) {
	PacketHandlerIterator firstHandler;
	PacketHandlerIterator lastHandler;
	bool hasHandler = GetPacketHandlers(packet->type, firstHandler, lastHandler);
	if (!hasHandler) {
		std::cerr << __FUNCTION__ << " no handler for packet type " << packet->type << std::endl;
		return false;
	}

	for (auto i = firstHandler; i != lastHandler; i++) {
		i->second->ReceivePacket(packet->type, packet, peerID);
	}
	return true;
}