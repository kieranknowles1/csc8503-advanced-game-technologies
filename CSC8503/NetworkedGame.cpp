#include "NetworkedGame.h"

#include <iostream>
#include <string>
#include <algorithm>

#include "PositionConstraint.h"
#include "OrientationConstraint.h"

#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"
#include "RenderObject.h"
#include "Trapper.h"
#include "Kitten.h"
#include "Bonus.h"
#include "Trigger.h"

#define COLLISION_MSG 30

struct MessagePacket : public GamePacket {
	short playerID;
	short messageID;

	MessagePacket() : GamePacket(Type::Message) {
		size = sizeof(short) * 2;
	}
};

NetworkedGame::NetworkedGame(const Cli& cli)
	: cli(cli) {
	server = nullptr;
	client = nullptr;
	thisClient = nullptr;
	maze = nullptr;
	// Dummy net world, will be replaced by the server or client
	networkWorld = new NetworkWorld(nullptr, nullptr);

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;

	auto type = cli.getClientType();
	switch (type) {
		case Cli::ClientType::Server:
			StartAsServer();
			break;
		case Cli::ClientType::Client:
			StartAsClient(cli.getIp(), cli.getName());
			break;
		default:
			// Something to look at before connecting
			StartLevel();
			break;
	}
}

NetworkedGame::~NetworkedGame()	{
	ClearWorld();
	delete server;
	delete client;
	delete thisClient;
}

void NCL::CSC8503::NetworkedGame::decrementRemainingKittens(Kitten* kitten, NetworkPlayer* player)
{
	kittensSaved++;
	// Award points to the player
	int id = player->getClientID();
	setPlayerScore(id, getPlayerScore(id) + 300);
	if (server) {
		server->sendPlayerList();
	}
}

void NetworkedGame::StartAsServer() {
	server = new Server(this, MaxPlayers);
	delete networkWorld;
	networkWorld = new NetworkWorld(thisClient, server->getServer());

	localPlayerId = HostPlayerId;
	auto state = generateNetworkState(localPlayerId, cli.getName());
	allPlayers.emplace(localPlayerId, LocalPlayerState(state));

	timeLimit = cli.getMaxGameLength();
	StartLevel();
}

void NetworkedGame::StartAsClient(uint32_t addr, std::string_view name) {
	connectionFailed = false;
	connectionLength = 0.0f;
	thisClient = new GameClient();
	thisClient->Connect(addr, NetworkBase::GetDefaultPort());

	if (!thisClient->isConnected()) {
		connectionFailed = true;
		delete thisClient;
		thisClient = nullptr;
		return;
	}

	delete networkWorld;
	networkWorld = new NetworkWorld(thisClient, nullptr);

	ClientHelloPacket packet;
	packet.name.set(name);
	thisClient->SendPacket(packet);

	thisClient->RegisterPacketHandler(GamePacket::Type::Reset, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::GameEnd, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::PlayerDisconnected, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::PlayerList, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::ServerHello, this);
	thisClient->RegisterPacketHandler(GamePacket::Type::ObjectDestroy, this);

	StartLevel();
}

void NetworkedGame::drawEndScreen() {
	Debug::Print(server
		? "Game Over! Press F11 to restart"
		: "Game Over! Waiting for server to restart",
		Vector2(10, 10)
	);

	LocalPlayerState* winner = nullptr;
	for (auto& player : allPlayers) {
		if (winner == nullptr || player.second.netState.score > winner->netState.score) {
			winner = &player.second;
		}
	}
	Debug::Print(winner->player->GetName() + " wins", Vector2(10, 15));
	drawScoreboard();
}

void NetworkedGame::drawScoreboard() {
	float yPos = 20;
	float lineHeight = 5;
	for (auto& [id, player] : allPlayers) {
		// string_view doesn't support concatenation :(
		std::string name(player.netState.name.get());
		std::string score = std::to_string(player.netState.score);
		Vector4 color = id == localPlayerId ? Vector4(1, 1, 1, 1) : Vector4(0.5, 0.5, 0.5, 1);
		Debug::Print(name + ": " + score, Vector2(10, yPos), color);
		yPos += lineHeight;
	}
	yPos += lineHeight;

	TaggedObjects::iterator first; TaggedObjects::iterator last;
	world->getTaggedObjects(GameObject::Tag::Bonus, first, last);
	int bonusesRemaining = std::distance(first, last);

	Debug::Print("Bonuses collected: " + std::to_string(totalBonusCount - bonusesRemaining) + "/" + std::to_string(totalBonusCount), Vector2(10, yPos));
	yPos += lineHeight;

	Debug::Print("Kittens safe: " + std::to_string(kittensSaved) + "/" + std::to_string(totalKittenCount), Vector2(10, yPos));
	yPos += lineHeight;

	int seconds = (int)timeLimit - (int)timeElapsed;
	int minutes = seconds / 60;
	seconds %= 60;

	std::stringstream ss;
	ss << "Time remaining: " << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;
	Debug::Print(ss.str(), Vector2(10, yPos));
	yPos += lineHeight;
}

void NetworkedGame::drawMainMenu() {
	Debug::Print(
		"Small Feline Big Deadline\n"
		"\n"
		"Press F4 to start a server\n"
		"Or F5 to connect to a server\n",
		Vector2(10, 10)
	);

	if (connectionFailed) {
		Debug::Print("Failed to connect to server", Vector2(10, 30), Vector4(1, 0, 0, 1));
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F4)) {
		StartAsServer();
	} else if (Window::GetKeyboard()->KeyPressed(KeyCodes::F5)) {
		StartAsClient(cli.getIp(), cli.getName());
	}
}

void NetworkedGame::UpdateGame(float dt) {
	if (server == nullptr && thisClient == nullptr) {
		drawMainMenu();
		// Run the rest of the loop to get rendering and a basic menu background
		//return;
	}

	timeElapsed = std::min(timeElapsed + dt, timeLimit);

	if (server && !gameEnded) {
		gameEnded = totalKittenCount == kittensSaved || Window::GetKeyboard()->KeyPressed(KeyCodes::F10) || timeElapsed >= timeLimit;
		if (gameEnded) {
			server->getServer()->SendGlobalPacket(GamePacket::Type::GameEnd);
		}
	}

	if (gameEnded) {
		drawEndScreen();
	}

	timeToNextPacket -= dt;

	if (Window::GetKeyboard()->KeyDown(KeyCodes::TAB))
		drawScoreboard();

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

	clearGraveyard();
}

void NetworkedGame::removeObject(GameObject* obj)
{
	graveyard.push_back(obj);
}

PlayerState NetworkedGame::generateNetworkState(int clientId, std::string_view name)
{
	PlayerState state;
	state.colour = generateCatColor();
	state.id = clientId;
	state.name.set(name);
	state.netObjectID = clientId + PlayerIdStart;
	state.score = 0;
	return state;
}

void NetworkedGame::clearGraveyard() {
	for (auto obj : graveyard) {
		if (server) {
			server->broadcastObjectDestroy(obj->GetNetworkObject()->getId());
		}

		networkWorld->removeObject(obj);
		physics->removeObject(obj);
		world->RemoveGameObject(obj, true);
	}
	graveyard.clear();
}

void NetworkedGame::ProcessInput(float dt) {
	auto it = allPlayers.find(localPlayerId);
	if (it == allPlayers.end()) {
		return;
	}
	auto playerObject = it->second.player;
	if (playerObject == nullptr) {
		return;
	}

	auto input = playerObject->processInput();

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F2)) {
		freeCam = !freeCam;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F3)) {
		Debug::setLinesEnabled(!Debug::getLinesEnabled());
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
	for (auto i : world->objects()) {
		NetworkObject* o = i->GetNetworkObject();
		if (!o) {
			continue;
		}
		o->UpdateStateHistory(minID); //clear out old states so they arent taking up memory...
	}
}

NetworkPlayer* NetworkedGame::SpawnPlayer(PlayerState state) {
	auto obj = AddPlayerToWorld(Vector3(0, 5, 0), state.id);
	obj->GetRenderObject()->SetColour(state.colour);
	obj->SetName(state.name.get());
	networkWorld->trackObjectManual(obj, state.netObjectID);

	return obj;
}

void NetworkedGame::SpawnMissingPlayers() {
	for (auto& [playerId, playerState] : allPlayers) {
		if (playerState.player == nullptr) {
			playerState.player = SpawnPlayer(playerState.netState);
		}
	}
}

void NetworkedGame::ClearWorld() {
	clearGraveyard();
	TutorialGame::ClearWorld();
	networkWorld->reset();
	physics->dirtyStaticsTree();

	for (auto& player : allPlayers) {
		player.second.player = nullptr;
		player.second.netState.score = 0;
	}
	delete maze; maze = nullptr;
	totalBonusCount = 0;
	totalKittenCount = 0;
	kittensSaved = 0;
	gameEnded = false;
}

GameObject* NCL::CSC8503::NetworkedGame::AddKittenToWorld(const Vector3& position)
{
	Kitten* kitten = new Kitten(kittenMesh, basicShader, basicTex, this);
	kitten->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(0.5, 0.5, 0.5));
	kitten->GetRenderObject()->SetColour(generateCatColor());
	world->AddGameObject(kitten);
	networkWorld->trackObject(kitten);

	totalKittenCount++;
	return kitten;
}

GameObject* NCL::CSC8503::NetworkedGame::AddNetworkCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, bool axisAligned)
{
	auto cube = AddCubeToWorld(position, dimensions, inverseMass, axisAligned);
	networkWorld->trackObject(cube);
	return cube;
}

void NetworkedGame::AddHomeToWorld(const Vector3& position, const Vector3& dimensions) {
	auto floor = new GameObject();
	floor->GetTransform()
		.SetPosition(position - Vector3(0, 2.95, 0))
		.SetScale(dimensions);
	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, nullptr, basicShader));
	world->AddGameObject(floor);

	// Higher order function to track when we're in the trigger
	auto triggerFunc = [&](bool state) {
		return [=](GameObject* other) {
			if (auto player = dynamic_cast<NetworkPlayer*>(other)) {
				// The kitten uses this to check if it's safe
				player->setInHome(state);
			}
		};
	};

	auto trigger = new Trigger(triggerFunc(true), triggerFunc(false));
	trigger->SetBoundingVolume(new AABBVolume(dimensions / 2.0f));
	trigger->GetTransform()
		.SetPosition(position + (dimensions * Vector3(0, 0.5, 0)))
		.SetScale(dimensions);
	trigger->SetPhysicsObject(new PhysicsObject(&trigger->GetTransform(), trigger->GetBoundingVolume()));
	trigger->GetPhysicsObject()->SetInverseMass(0.0f);
	world->AddGameObject(trigger);
}

GameObject* NetworkedGame::AddBridgeToWorld(const BridgeSettings& settings) {
	GameObject* start = AddNetworkCubeToWorld(settings.start, settings.nodeSize, 0.0f);
	GameObject* end = AddNetworkCubeToWorld(settings.end, settings.nodeSize, 0.0f);
	Vector3 spacing = (settings.end - settings.start) / (settings.linkCount + 1);

	GameObject* previous = start;
	for (int i = 0; i < settings.linkCount; i++) {
		Vector3 thisPos = settings.start + spacing * (i + 1);
		GameObject* block = AddNetworkCubeToWorld(thisPos, settings.nodeSize, settings.nodeInverseMass);
		block->GetPhysicsObject()->SetElasticity(0.01);
		PositionConstraint* constraint = new PositionConstraint(previous, block, settings.nodeMaxDistance, PositionConstraint::Type::Rope);
		world->AddConstraint(constraint);
		OrientationConstraint* oConstraint = new OrientationConstraint(previous, block, Quaternion(), Vector3(-10, -10, -10), Vector3(10, 10, 10));
		world->AddConstraint(oConstraint);
		previous = block;
	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, settings.nodeMaxDistance, PositionConstraint::Type::Rope);
	world->AddConstraint(constraint);
	OrientationConstraint* oConstraint = new OrientationConstraint(previous, end, Quaternion(), Vector3(-10, -10, -10), Vector3(10, 10, 10));
	world->AddConstraint(oConstraint);

	return end;
}

Bonus* NetworkedGame::AddBonusToWorld(const Vector3& position) {
	Bonus* apple = new Bonus(this, 100, rng);
	apple->SetTrigger(true);

	SphereVolume* volume = new SphereVolume(1.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(0.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	totalBonusCount++;
	return apple;
}

void NetworkedGame::StartLevel() {
	ClearWorld();

	AddFloorToWorld(Vector3(0, 0, 0));

	AddHomeToWorld(Vector3(-20, 0, 0), Vector3(10, 10, 10));

	auto netCube = AddCubeToWorld(Vector3(-10, 20, 0), Vector3(1, 1, 5), 0.5f);
	networkWorld->trackObject(netCube);

	auto bonus = AddBonusToWorld(Vector3(10, 2.5, 0));
	networkWorld->trackObject(bonus);

	AddKittenToWorld(Vector3(0, 5, 5));

	BridgeSettings bridgeSettings;
	bridgeSettings.start = Vector3(200, -5, -50);
	bridgeSettings.end = Vector3(400, -5, -100);
	bridgeSettings.nodeSize = Vector3(4, 1.5, 4);
	// 1kg/m^3 density
	bridgeSettings.nodeInverseMass = 1.0f / boxVolume(bridgeSettings.nodeSize);
	bridgeSettings.linkCount = 20;
	bridgeSettings.nodeMaxDistance = 12;

	GameObject* bridgeEnd = AddBridgeToWorld(bridgeSettings);
	bridgeSettings.start.z = bridgeSettings.end.z + (bridgeSettings.end.z - bridgeSettings.start.z);
	AddBridgeToWorld(bridgeSettings);
	auto bridgeBonus = AddBonusToWorld(
		bridgeEnd->GetTransform().GetPosition()
		+ bridgeEnd->GetTransform().GetScale() * Vector3(0, 0.75, 0)
	);
	bridgeBonus->setValue(300); // Quite difficult to get to
	networkWorld->trackObject(bridgeBonus);

	maze = new NavigationGrid("maze.txt", Vector3(32, 0, 32));
	int nodeSize = maze->getNodeSize();
	for (int i = 0; i < maze->getNodeCount(); i++) {
		GridNode* node = maze->getNode(i);
		switch (node->type) {
		case GridNode::Type::Wall:
			AddCubeToWorld(node->position + Vector3(0, 10, 0), Vector3(nodeSize / 2, 10, nodeSize / 2), 0.0f, true);
			break;
		case GridNode::Type::Bonus: {
			auto bonus = AddBonusToWorld(node->position + Vector3(0, 2.5, 0));
			networkWorld->trackObject(bonus);
			// TODO: Reward for collecting
			break;
		} case GridNode::Type::Enemy: {
			auto enemy = new Trapper(rng, enemyMesh, basicShader, maze, world);
			enemy->GetTransform().SetPosition(node->position);
			enemy->SetDefaultTransform(enemy->GetTransform());
			world->AddGameObject(enemy);
			networkWorld->trackObject(enemy);
			break;
		} case GridNode::Type::Kitten: {
			AddKittenToWorld(node->position + Vector3(0, 5, 0));
			break;
		} default:
			break;
		}
	}

	physics->dirtyStaticsTree();
	SpawnMissingPlayers();
}

void NetworkedGame::ProcessPacket(PlayerDisconnectedPacket* payload) {
	std::cout << "Player " << payload->playerID << " disconnected\n";
}

void NetworkedGame::ProcessPacket(PlayerListPacket* payload) {
	std::cout << "Received list of " << (int)payload->count << " players\n";
	for (int i = 0; i < payload->count; i++) {
		auto state = payload->playerStates[i];
		std::cout << "Player " << state.id << " has object ID " << state.netObjectID << "\n";
		auto it = allPlayers.find(state.id);
		if (it == allPlayers.end()) {
			std::cout << "Spawning player " << state.name.get() << "\n";
			std::cout << "Player has colour " << state.colour << "\n";
			allPlayers.emplace(state.id, LocalPlayerState(state));
		}
		else {
			std::cout << "Updating player state for " << state.name.get() << "\n";
			it->second.netState = state;
		}
	}
	SpawnMissingPlayers();
}

void NetworkedGame::ProcessPacket(ServerHelloPacket* payload) {
	std::cout << "Received hello packet. We are player " << payload->whoAmI.id << "\n";
	std::cout << "Our colour is " << payload->whoAmI.colour << "\n";
	localPlayerId = payload->whoAmI.id;
	gameEnded = payload->gameEnded;
	timeElapsed = payload->timeElapsed;
	timeLimit = payload->timeLimit;
	allPlayers.emplace(localPlayerId, LocalPlayerState(payload->whoAmI));
}

void NCL::CSC8503::NetworkedGame::ProcessPacket(DestroyPacket* payload)
{
	removeObject(networkWorld->getTrackedObject(payload->id));
}

void NetworkedGame::ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) {
	switch (type)
	{
	case GamePacket::Type::PlayerDisconnected:
		return ProcessPacket((PlayerDisconnectedPacket*)payload);
	case GamePacket::Type::PlayerList:
		return ProcessPacket((PlayerListPacket*)payload);
	case GamePacket::Type::ServerHello:
		return ProcessPacket((ServerHelloPacket*)payload);
	case GamePacket::Type::ObjectDestroy:
		return ProcessPacket((DestroyPacket*)payload);
	case GamePacket::Type::Reset:
		StartLevel();
		break;
	case GamePacket::Type::GameEnd:
		gameEnded = true;
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
