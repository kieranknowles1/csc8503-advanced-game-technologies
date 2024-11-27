#include "NetworkedGame.h"
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

NetworkedGame::NetworkedGame()	{
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;
}

NetworkedGame::~NetworkedGame()	{
	delete thisServer;
	delete thisClient;
}

void NetworkedGame::StartAsServer() {
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 128);

	thisServer->RegisterPacketHandler(GamePacket::Type::ClientState, this);

	StartLevel();
}

void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
	connectionLength = 0.0f;
	thisClient = new GameClient();
	thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

	thisClient->RegisterPacketHandler(GamePacket::Type::Delta_State, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::Full_State, this);
	// TODO
	//thisClient->RegisterPacketHandler(GamePacket::Type::Player_Connected, this);
	//thisClient->RegisterPacketHandler(GamePacket::Type::Player_Disconnected, this);

	StartLevel();
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

	if (!thisServer && Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
		StartAsServer();
	}
	if (!thisClient && Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
		StartAsClient(127,0,0,1);
	}

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::UpdateAsServer(float dt) {
	thisServer->UpdateServer();

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

void NetworkedGame::SpawnPlayer() {

}

NetworkObject* NetworkedGame::createNetworkObject(GameObject* obj) {
	NetworkObject* netObj = new NetworkObject(*obj, nextNetworkId);
	obj->SetNetworkObject(netObj);
	networkObjects.emplace(nextNetworkId, obj);
	nextNetworkId++;
	return netObj;
}

GameObject* NetworkedGame::getNetworkObject(NetworkObject::Id id) {
	auto i = networkObjects.find(id);
	// TODO: This doesn't handle destruction, may cause use-after-free
	if (i == networkObjects.end()) {
		return nullptr;
	}
	return i->second;
}

void NetworkedGame::StartLevel() {
	ClearWorld();

	InitDefaultFloor();

	auto netCube = AddCubeToWorld(Vector3(0, 20, 0), Vector3(1, 1, 5), 0.5f);
	createNetworkObject(netCube);
}

void NetworkedGame::HandlePacket(DeltaPacket* payload, int source) {
	GameObject* o = getNetworkObject(payload->objectID);
	if (!o) {
		return;
	}
	o->GetNetworkObject()->ReadPacket(*payload);
}

void NetworkedGame::HandlePacket(FullPacket* payload, int source) {
	GameObject* o = getNetworkObject(payload->objectID);
	if (!o) {
		return;
	}
	o->GetNetworkObject()->ReadPacket(*payload);
}

void NetworkedGame::ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) {
	switch (type)
	{
	case GamePacket::Type::Delta_State:
		HandlePacket((DeltaPacket*)payload, source);
		break;
	case GamePacket::Type::Full_State:
		HandlePacket((FullPacket*)payload, source);
		break;
	//case GamePacket::Type::Player_Connected:
	//case GamePacket::Type::Player_Disconnected:
		// TODO
		//HandlePacket()
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
