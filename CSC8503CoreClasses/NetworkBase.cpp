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
		std::cerr << __FUNCTION__ << " no handler for packet type " << (short)packet->type << std::endl;
		return false;
	}

	for (auto i = firstHandler; i != lastHandler; i++) {
		i->second->ReceivePacket(packet->type, packet, peerID);
	}
	return true;
}

bool NetworkBase::ProcessPackedPackets(std::span<enet_uint8> buffer, int peerID)
{
	int count = 0;
	while (!buffer.empty()) {
		// Safety checks - ensure we have enough data to read the packet in full
		// If not, we have a buffer overrun and should stop processing
		if (buffer.size() < sizeof(GamePacket)) {
			throw std::runtime_error("Buffer overrun while reading packet header");
		}
		GamePacket* packet = reinterpret_cast<GamePacket*>(buffer.data());
		// A well-formed packet will have its size set correctly
		if (buffer.size() < packet->GetTotalSize()) {
			throw std::runtime_error("Buffer overrun while reading packet payload");
		}
		ProcessPacket(packet, peerID);
		buffer = buffer.subspan(packet->GetTotalSize());
		count++;
	}
	return true;
}
