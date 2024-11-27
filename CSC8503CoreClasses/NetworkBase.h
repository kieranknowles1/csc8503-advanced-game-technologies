#pragma once
//#include "./enet/enet.h"
struct _ENetHost;
struct _ENetPeer;
struct _ENetEvent;

struct GamePacket {
	enum class Type : short {
		// Null terminated string
		// @see: StringPacket
		String_Message,
		// Delta state of a network object
		// @see: DeltaPacket
		Delta_State,
		// Full state of a network object
		// @see: FullPacket
		Full_State,
		// Client input state
		// @see: ClientPacket
		ClientState,
		// Payloadless message intended for a specific player
		Message,

		PayloadEnd, // Marker for packets that have payloads

		// Server has shut down, all clients should disconnect
		Shutdown,
		// Level reset
		Reset,


		// None,
		// Hello,
		// Message,
		// String_Message,
		// Delta_State,	//1 byte per channel since the last state
		// Full_State,		//Full transform etc
		// ClientState, //received from a client, informs that its received packet n
		// Player_Connected,
		// Player_Disconnected,
		// Shutdown
	};

	short size;
	Type type;

	GamePacket(Type type)
		: size(0)
		, type(type)
	{}

	int GetTotalSize() {
		return sizeof(GamePacket) + size;
	}
};

struct StringPacket : public GamePacket {
	// Null terminated string
	char data[256];

	StringPacket(std::string_view message) : GamePacket(Type::String_Message) {
		if (message.length() > sizeof(data)) {
			throw std::runtime_error("String too long for packet data!");
		};

		size = message.length() + 1;
		memcpy(data, message.data(), message.length());
		data[message.length()] = 0;
	}

	std::string toString() const {
		return std::string(data);
	}
};

class PacketReceiver {
public:
	virtual void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source = -1) = 0;
};

class NetworkBase	{
public:
	static void Initialise();
	static void Destroy();

	static int GetDefaultPort() {
		return 1234;
	}

	void RegisterPacketHandler(GamePacket::Type msgID, PacketReceiver* receiver) {
		packetHandlers.insert(std::make_pair(msgID, receiver));
	}
protected:
	NetworkBase();
	~NetworkBase();

	bool ProcessPacket(GamePacket* p, int peerID = -1);

	using PacketHandlerMap = std::multimap<GamePacket::Type, PacketReceiver*>;
	using PacketHandlerIterator = PacketHandlerMap::const_iterator;

	bool GetPacketHandlers(GamePacket::Type msgID, PacketHandlerIterator& first, PacketHandlerIterator& last) const {
		auto range = packetHandlers.equal_range(msgID);

		if (range.first == packetHandlers.end()) {
			return false; //no handlers for this message type!
		}
		first	= range.first;
		last	= range.second;
		return true;
	}

	_ENetHost* netHandle;

	PacketHandlerMap packetHandlers;
};
