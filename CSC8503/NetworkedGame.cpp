#include "NetworkedGame.h"

#include <iostream>
#include <string>
#include <algorithm>

#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"

#define COLLISION_MSG 30

struct MessagePacket : public GamePacket {
	short playerID;
	short messageID;

	MessagePacket() : GamePacket(Type::Message) {
		size = sizeof(short) * 2;
	}
};

NetworkedGame::NetworkedGame(const Cli& cli) {
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;

	if (cli.isServer()) {
		StartAsServer();
	}
	else {
		// TODO: Allow specifying IP
		StartAsClient(127, 0, 0, 1);
	}

	networkWorld = new NetworkWorld(thisClient, thisServer);

	if (thisServer) {
		int playerId = -1;
		auto obj = SpawnPlayer(PlayerIdStart + playerId);
		localPlayer = PlayerState{ playerId, obj->GetNetworkObject()->getId() };
		allPlayers[playerId] = localPlayer;
	}

	StartLevel();
}

NetworkedGame::~NetworkedGame()	{
	delete thisServer;
	delete thisClient;
}

void NetworkedGame::StartAsServer() {
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), MaxPlayers);

	thisServer->RegisterPacketHandler(GamePacket::Type::ClientState, this);
	thisServer->RegisterPacketHandler(GamePacket::Type::Server_ClientConnect, this);
	thisServer->RegisterPacketHandler(GamePacket::Type::Server_ClientDisconnect, this);
}

void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
	connectionLength = 0.0f;
	thisClient = new GameClient();
	thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

	// TODO
	//thisClient->RegisterPacketHandler(GamePacket::Type::Player_Connected, this);
	//thisClient->RegisterPacketHandler(GamePacket::Type::Player_Disconnected, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::Reset, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::PlayerConnected, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::PlayerDisconnected, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::PlayerList, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::Hello, this);
}

void NetworkedGame::UpdateGame(float dt) {
	timeToNextPacket -= dt;
	if (thisServer)
		Debug::Print("This is a server with " + std::to_string(thisServer->getClientCount()) + " clients\n", Vector2(10, 10));
	if (thisClient)
		Debug::Print("This is a client\n", Vector2(10, 20));

	if (timeToNextPacket < 0) {
		if (thisServer) {
			UpdateAsServer(dt);
		} else if (thisClient) {
			UpdateAsClient(dt);
		}
		timeToNextPacket += 1.0f / 20.0f; //20hz server/client update
	}

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::UpdateAsServer(float dt) {
	thisServer->UpdateServer();

	if (Window::GetKeyboard()->KeyDown(KeyCodes::F11)) {
		auto reset = GamePacket(GamePacket::Type::Reset);
		thisServer->SendGlobalPacket(reset);
		StartLevel();
	}

	packetsToSnapshot--;
	if (packetsToSnapshot < 0) {
		BroadcastSnapshot(false);
		packetsToSnapshot = 5;
	}
	else {
		BroadcastSnapshot(true);
	}
}

void NetworkedGame::UpdateAsClient(float dt) {
	// Very simple check for a timeout - just disconnect and delete the client
	if (connectionLength > 1.0f && !thisClient->isConnected()) {
		std::cerr << "Connection timed out!\n";
		delete thisClient;
		thisClient = nullptr;
		return;
	}
	connectionLength += dt;

	thisClient->UpdateClient();

	ClientPacket newPacket;

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
		//fire button pressed!
		newPacket.buttonstates[0] = 1;
		// TODO: Set this somehow
		newPacket.lastID = 0; //You'll need to work this out somehow...
	}
	thisClient->SendPacket(newPacket);
}

void NetworkedGame::BroadcastSnapshot(bool deltaFrame) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;

	world->GetObjectIterators(first, last);

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
		int playerState = 0;
		GamePacket* newPacket = nullptr;
		if (o->WritePacket(&newPacket, deltaFrame, playerState)) {
			thisServer->SendGlobalPacket(*newPacket);
			delete newPacket;
		}
	}
}

void NetworkedGame::UpdateMinimumState() {
	//Periodically remove old data from the server
	int minID = INT_MAX;
	int maxID = 0; //we could use this to see if a player is lagging behind?

	for (auto i : stateIDs) {
		minID = std::min(minID, i.second);
		maxID = std::max(maxID, i.second);
	}
	//every client has acknowledged reaching at least state minID
	//so we can get rid of any old states!
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		o->UpdateStateHistory(minID); //clear out old states so they arent taking up memory...
	}
}

GameObject* NetworkedGame::SpawnPlayer(int id) {
	auto obj = AddPlayerToWorld(Vector3(0, 5, 0));
	networkWorld->trackObjectManual(obj, id);

	return obj;
}

void NetworkedGame::SpawnMissingPlayers() {
	for (auto& [playerID, playerState] : allPlayers) {
		auto netObj = networkWorld->getTrackedObject(playerState.netObjectID);
		if (!netObj) {
			auto obj = SpawnPlayer(playerState.netObjectID);
		}
	}
}

void NetworkedGame::StartLevel() {
	ClearWorld();
	networkWorld->reset();

	InitDefaultFloor();

	auto netCube = AddCubeToWorld(Vector3(0, 20, 0), Vector3(1, 1, 5), 0.5f);
	networkWorld->trackObject(netCube);

	SpawnMissingPlayers();
}

//void NetworkedGame::ProcessPacket(PlayerConnectedPacket* payload) {
//	std::cout << "Player " << payload->playerID << " connected\n";
//	auto obj = SpawnPlayer(payload->playerObjectID);
//	allPlayers[payload->playerID] = obj;
//}

void NetworkedGame::ProcessPacket(PlayerDisconnectedPacket* payload) {
	std::cout << "Player " << payload->playerID << " disconnected\n";
}

void NetworkedGame::ProcessPacket(PlayerListPacket* payload) {
	std::cout << "Received list of " << (int)payload->count << " players\n";
	for (int i = 0; i < payload->count; i++) {
		auto player = payload->playerStates[i];
		std::cout << "Player " << player.id << " has object ID " << player.netObjectID << "\n";
		allPlayers[player.id] = player;
	}
	SpawnMissingPlayers();
}

void NetworkedGame::ProcessPacket(HelloPacket* payload) {
	std::cout << "Received hello packet. We are player " << payload->whoAmI.id << "\n";
	localPlayer = payload->whoAmI;
}

void NetworkedGame::ProcessPlayerConnect(int playerID)
{
	std::cout << "Player " << playerID << " connected\n";
	// TODO: Implement
	auto playerObject = SpawnPlayer(PlayerIdStart + playerID);
	allPlayers[playerID] = PlayerState{
		playerID,
		playerObject->GetNetworkObject()->getId()
	};

	PlayerConnectedPacket newPacket(playerID, playerObject);
	thisServer->SendGlobalPacket(newPacket);

	PlayerListPacket listPacket(allPlayers);
	thisServer->SendGlobalPacket(listPacket);

	HelloPacket helloPacket{
		PlayerState{playerID, playerObject->GetNetworkObject()->getId()}
	};
	thisServer->SendClientPacket(playerID, helloPacket);
}

void NetworkedGame::ProcessPlayerDisconnect(int playerID)
{
	std::cout << "Player " << playerID << " disconnected\n";
	// TODO: Implement
}

void NetworkedGame::ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) {
	switch (type)
	{
	case GamePacket::Type::Server_ClientConnect:
		return ProcessPlayerConnect(source);
	case GamePacket::Type::Server_ClientDisconnect:
		return ProcessPlayerDisconnect(source);
	case GamePacket::Type::PlayerConnected:
	//	return ProcessPacket((PlayerConnectedPacket*)payload);
	//case GamePacket::Type::PlayerDisconnected:
		return ProcessPacket((PlayerDisconnectedPacket*)payload);
	case GamePacket::Type::PlayerList:
		return ProcessPacket((PlayerListPacket*)payload);
	case GamePacket::Type::Hello:
		return ProcessPacket((HelloPacket*)payload);
	case GamePacket::Type::Reset:
		StartLevel();
		break;
	default:
		break;
	}
}

void NetworkedGame::OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b) {
	if (thisServer) { //detected a collision between players!
		MessagePacket newPacket;
		newPacket.messageID = COLLISION_MSG;
		newPacket.playerID  = a->GetPlayerNum();

		thisClient->SendPacket(newPacket);

		newPacket.playerID = b->GetPlayerNum();
		thisClient->SendPacket(newPacket);
	}
}
