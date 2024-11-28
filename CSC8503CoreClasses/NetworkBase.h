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

		// A new player has connected
		// The first of this type seen by a client is the client's own connection
		// @see: PlayerConnectedPacket
		PlayerConnected,
		// A player has disconnected
		// @see: PlayerDisconnectedPacket
		PlayerDisconnected,

		// A list of all players in the game, along with their current state
		// @see: PlayerListPacket
		PlayerList,

		PayloadEnd, // Marker for packets that have payloads

		// Only sent to servers, a new client has connected
		Server_ClientConnect,
		// Only sent to servers, a client has disconnected
		Server_ClientDisconnect,

		// Only sent to clients, the server has accepted the connection
		Client_ClientConnect,
		// Only sent to clients, the server has disconnected
		Client_ClientDisconnect,

		// Server has shut down, all clients should disconnect
		Shutdown,
		// Level reset
		Reset,

		// Hello,
		// Message,
		// ClientState, //received from a client, informs that its received packet n
		// Player_Connected,
		// Player_Disconnected,
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
