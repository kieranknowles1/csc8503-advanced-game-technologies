#include "NetworkedGame.h"

#include <iostream>
#include <string>
#include <algorithm>

#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"
#include "Trapper.h"

#define COLLISION_MSG 30

struct MessagePacket : public GamePacket {
	short playerID;
	short messageID;

	MessagePacket() : GamePacket(Type::Message) {
		size = sizeof(short) * 2;
	}
};

NetworkedGame::NetworkedGame(const Cli& cli) {
	server = nullptr;
	client = nullptr;
	thisClient = nullptr;
	maze = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;

	if (cli.isServer()) {
		server = new Server(this, MaxPlayers);
	} else {
		StartAsClient(cli.getIp());
	}

	networkWorld = new NetworkWorld(thisClient, server ? server->getServer() : nullptr);

	if (server) {
		int playerId = -1;
		auto obj = SpawnPlayer(PlayerIdStart + playerId);
		localPlayer = PlayerState{ playerId, obj->GetNetworkObject()->getId() };
		allPlayers[playerId] = {
			localPlayer,
			obj
		};
	}

	StartLevel();
}

NetworkedGame::~NetworkedGame()	{
	ClearWorld();
	delete server;
	delete client;
	delete thisClient;
}

void NetworkedGame::StartAsClient(uint32_t addr) {
	connectionLength = 0.0f;
	thisClient = new GameClient();
	thisClient->Connect(addr, NetworkBase::GetDefaultPort());

	thisClient->RegisterPacketHandler(GamePacket::Type::Reset, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::PlayerConnected, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::PlayerDisconnected, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::PlayerList, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::Hello, this);
}

void NetworkedGame::UpdateGame(float dt) {
	timeToNextPacket -= dt;
	if (server)
		Debug::Print("This is a server with " + std::to_string(server->getServer()->getClientCount()) + " clients\n", Vector2(10, 10));
	if (thisClient)
		Debug::Print("This is a client\n", Vector2(10, 20));

	if (timeToNextPacket < 0) {
		if (server) {
			server->update(dt);
		} else if (thisClient) {
			UpdateAsClient(dt);
		}
		timeToNextPacket += inverseTickRate;
	}

	ProcessInput(dt);

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::ProcessInput(float dt) {
	auto it = allPlayers.find(localPlayer.id);
	if (it == allPlayers.end()) {
		return;
	}
	auto playerObject = it->second.player;

	auto input = playerObject->processInput();

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F2)) {
		freeCam = !freeCam;
	}

	// Track the player with the camera
	lockedObject = freeCam ? nullptr : playerObject;

	if (server) {
		playerObject->setLastInput(input);
	} else {
		ClientPacket newPacket;
		newPacket.input = input;
		// TODO: Should this sync with the state ID?
		newPacket.index = inputIndex++;
		thisClient->SendPacket(newPacket);
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

	// ClientPacket newPacket;

	// if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
	// 	//fire button pressed!
	// 	newPacket.buttonstates[0] = 1;
	// 	// TODO: Set this somehow
	// 	newPacket.lastID = 0; //You'll need to work this out somehow...
	// }
	// thisClient->SendPacket(newPacket);
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

		// TODO: Set to the last state that all players have acknowledged
		int playerState = o->GetLastFullState().stateID;
		GamePacket* newPacket = nullptr;
		if (o->WritePacket(&newPacket, deltaFrame, playerState)) {
			server->getServer()->SendGlobalPacket(*newPacket);
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

NetworkPlayer* NetworkedGame::insertPlayer(int index) {
	auto obj = SpawnPlayer(index + PlayerIdStart);
	allPlayers[index] = {
		PlayerState(index, obj->GetNetworkObject()->getId()),
		obj
	};
	return obj;
}

NetworkPlayer* NetworkedGame::SpawnPlayer(int id) {
	auto obj = AddPlayerToWorld(Vector3(0, 5, 0));
	networkWorld->trackObjectManual(obj, id);

	return obj;
}

void NetworkedGame::SpawnMissingPlayers() {
	for (auto& [playerID, playerState] : allPlayers) {
		if (playerState.player == nullptr) {
			playerState.player = SpawnPlayer(playerState.netState.netObjectID);
		}
	}
}

void NetworkedGame::ClearWorld() {
	TutorialGame::ClearWorld();
	networkWorld->reset();
	physics->dirtyStaticsTree();

	for (auto& player : allPlayers) {
		player.second.player = nullptr;
	}
	delete maze; maze = nullptr;
}

void NetworkedGame::StartLevel() {
	ClearWorld();

	AddFloorToWorld(Vector3(0, 0, 0));

	auto netCube = AddCubeToWorld(Vector3(0, 20, 0), Vector3(1, 1, 5), 0.5f);
	networkWorld->trackObject(netCube);

	maze = new NavigationGrid("maze.txt", Vector3(32, 0, 32));
	int nodeSize = maze->getNodeSize();
	for (int i = 0; i < maze->getNodeCount(); i++) {
		GridNode* node = maze->getNode(i);
		if (node->type == WALL_NODE) {
			AddCubeToWorld(node->position, Vector3(nodeSize/2, nodeSize, nodeSize/2), 0.0f, true);
		}
	}

	auto trapper = new Trapper(rng, enemyMesh, basicShader, maze);
	world->AddGameObject(trapper);
	networkWorld->trackObject(trapper);

	physics->dirtyStaticsTree();
	SpawnMissingPlayers();
}

void NetworkedGame::ProcessPacket(PlayerDisconnectedPacket* payload) {
	std::cout << "Player " << payload->playerID << " disconnected\n";
}

void NetworkedGame::ProcessPacket(PlayerListPacket* payload) {
	std::cout << "Received list of " << (int)payload->count << " players\n";
	for (int i = 0; i < payload->count; i++) {
		auto player = payload->playerStates[i];
		std::cout << "Player " << player.id << " has object ID " << player.netObjectID << "\n";
		if (allPlayers.find(player.id) == allPlayers.end()) {
			std::cout << "Spawning player " << player.id << "\n";
			allPlayers[player.id] = { player, nullptr };
		}
	}
	SpawnMissingPlayers();
}

void NetworkedGame::ProcessPacket(HelloPacket* payload) {
	std::cout << "Received hello packet. We are player " << payload->whoAmI.id << "\n";
	localPlayer = payload->whoAmI;
}

void NetworkedGame::ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) {
	switch (type)
	{
	// Client events
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

//void NetworkedGame::OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b) {
//	if (thisServer) { //detected a collision between players!
//		MessagePacket newPacket;
//		newPacket.messageID = COLLISION_MSG;
//		newPacket.playerID  = a->GetPlayerNum();
//
//		thisClient->SendPacket(newPacket);
//
//		newPacket.playerID = b->GetPlayerNum();
//		thisClient->SendPacket(newPacket);
//	}
//}
