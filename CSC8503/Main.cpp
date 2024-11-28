#include "Window.h"

#include <iostream>

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"
#include "Cli.h"

#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"
#include "BehaviourParallel.h"
#include "BehaviourInverter.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>

NavigationPath TestPathfinding() {
	NavigationGrid grid("TestGrid1.txt");
	NavigationPath outPath;

	Vector3 startPos(80, 0, 10);
	Vector3 end(80, 0, 80);
	Debug::DrawLine(startPos, end, Debug::BLUE);

	bool found = grid.FindPath(startPos, end, outPath);
	return outPath;
}

void DisplayPathfinding() {
	auto path = TestPathfinding();
	std::vector<Vector3> nodes;
	Vector3 pos;
	while (path.PopWaypoint(pos)) nodes.push_back(pos);

	for (int i = 1; i < nodes.size(); i++) {
		Vector3 from = nodes[i - 1];
		Vector3 to = nodes[i];
		Debug::DrawLine(from, to, Vector4(0, 1, 0, 1));
	}
}

// Basic state machine that oscillates between two states
void testStateMachine() {
	StateMachine* machine = new StateMachine();
	int data = 0;

	State* a = new State([&](float dt)->void {
		std::cout << "Guten tag! Ich bin ein state A!" << std::endl;
		data++;
	});
	State* b = new State([&](float dt)-> void {
		std::cout << "Bonjour! Je suis un state B!" << std::endl;
		data--;
	});

	StateTransition* aToB = new StateTransition(a, b, [&]()->bool {
		return data > 10;
	});
	StateTransition* bToA = new StateTransition(b, a, [&]()->bool {
		return data < 0;
	});

	machine->AddState(a);
	machine->setStartingState(a);
	machine->AddState(b);
	machine->AddTransition(aToB);
	machine->AddTransition(bToA);

	for (int i = 0; i < 100; i++) {
		machine->Update(1.0f);
	}
	delete machine; // The machine owns its states and transitions, so it will delete them too
}

void testBehaviourTree() {
	float timer;
	float distanceToTarget;
	BehaviourAction* findKey = new BehaviourAction("FindKey", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Finding key!" << std::endl;
			timer = rand() % 100;
			return Ongoing;
		}
		else if (state == Ongoing) {
			timer -= dt;
			if (timer <= 0.0f) {
				std::cout << "Found a key" << std::endl;
				return Success;
			}
		}
		return state; // Continue searching
	});

	// Actions can take some time to complete
	BehaviourAction* goToRoom = new BehaviourAction("GoToRoom", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Fetching loot!" << std::endl;
			return Ongoing;
		} else if (state == Ongoing) {
			distanceToTarget -= dt;
			if (distanceToTarget <= 0.0f) {
				std::cout << "Got to the room!" << std::endl;
				return Success;
			}
		}
		return state;
	});

	// Actions may be instant
	BehaviourAction* openDoor = new BehaviourAction("OpenDoor", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Opening door!" << std::endl;
			return Success;
		}
		return state;
	});

	// Actions can succeed or fail
	BehaviourAction* lootThatBody = new BehaviourAction("Loot", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looting chest!" << std::endl;
			return Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "Found some loot!" << std::endl;
				std::cout << "1 gold added to inventory!" << std::endl;
				return Success;
			}
			std::cout << "No loot found!" << std::endl;
			std::cout << "lockpick removed from inventory!" << std::endl;
			return Failure;
		}
		return state;
	});

	BehaviourAction* packMule = new BehaviourAction("LootScrap", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Taking random junk!" << std::endl;
			return Ongoing;
		}
		else if (state == Ongoing) {
			bool overEncumbered = rand() % 2;
			// I'm not a loot goblin, I'm a loot dragon.
			// If it's not nailed down, I'm taking it.
			// If it is nailed down, I'm coming back with a hammer.
			std::cout << "1 kettle added to inventory!" << std::endl;
			std::cout << "3 well-seasoned iron pans added to inventory!" << std::endl;
			std::cout << "1 fork added to inventory!" << std::endl;
			if (overEncumbered) {
				std::cout << "You are carrying too much to be able to run!" << std::endl;
				return Failure;
			}
			std::cout << "Who even buys this stuff?" << std::endl;
			return Success;
		}
		return state;
	});

	// Actions that must be performed in sequence
	BehaviourSequence* sequence = new BehaviourSequence("RoomSequence");
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	// Do both actions at the same time until one succeeds or both fail
	BehaviourParallel* selection = new BehaviourParallel("LootSelector");
	selection->AddChild(lootThatBody);
	selection->AddChild(new BehaviourInverter("StrengthTraining", packMule));
	//selection->AddChild(packMule);

	// Do the first action that succeeds
	//BehaviourSelector* selection = new BehaviourSelector("LootSelector");
	//selection->AddChild(lootThatBody);
	//selection->AddChild(packMule);

	BehaviourSequence* root = new BehaviourSequence("RootSequence");
	root->AddChild(sequence);
	root->AddChild(selection);

	for (int i = 0; i < 20; i++) {
		root->Reset();
		timer = 0;
		distanceToTarget = rand() % 250;
		BehaviourState state = Ongoing;
		while (state == Ongoing) {
			state = root->Execute(1.0f);
		}
		if (state == Success) {
			std::cout << "Success!" << std::endl;
		}
		else {
			std::cout << "Failure!" << std::endl;
		}
	}
}

class PauseScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::U)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Press U to unpause!" << std::endl;
	};
};

class GameScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) {
		timer -= dt;
		if (timer < 0) {
			std::cout << "Coins: " << coins << std::endl;
			timer += 1.0f;
		}
		if (Window::GetKeyboard()->KeyDown(KeyCodes::P)) {
			*newState = new PauseScreen();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyDown(KeyCodes::F1)) {
			std::cout << "Exiting game!" << std::endl;
			return PushdownResult::Pop;
		}
		coins++;
		return PushdownResult::NoChange;
	}

	void OnAwake() {
		std::cout << "The children yearn for the mines!" << std::endl;
	}

	int coins = 0;
	float timer = 0.0f;
};

class IntroScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
			*newState = new GameScreen();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}

	void OnAwake() {
		std::cout << "Press SPACE to start!" << std::endl;
	}
};

void testPushdownAutomata(Window* window) {
	PushdownMachine machine(new IntroScreen());
	while (window->UpdateWindow()) {
		float dt = window->GetTimer().GetTimeDeltaSeconds();
		if (!machine.Update(dt)) {
			break;
		}
	}
}

class TestPacketReceiver : public PacketReceiver {
public:
	TestPacketReceiver(std::string name) {
		this->name = name;
	}

	void ReceivePacket(GamePacket::Type type, GamePacket* payload, int source) {
		if (type != GamePacket::Type::String_Message) {
			return;
		}

		StringPacket* realPacket = (StringPacket*)payload;
		std::cout << name << " received a message: " << realPacket->toString() << std::endl;
	}
protected:
	std::string name;
};

void testServer() {
	NetworkBase::Initialise();
	TestPacketReceiver serverReceiver("Server");
	int port = NetworkBase::GetDefaultPort();
	GameServer* server = new GameServer(port, 128);

	server->RegisterPacketHandler(GamePacket::Type::String_Message, &serverReceiver);

	while (server->getClientCount() < 2) {
		server->UpdateServer();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	for (int i = 0; i < 100; i++) {
		StringPacket p("Hello world!");
		server->SendGlobalPacket(p);
		server->UpdateServer();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	delete server;
	NetworkBase::Destroy();
}

void testClient() {
	NetworkBase::Initialise();
	TestPacketReceiver clientReceiver("Client");

	GameClient* client = new GameClient();
	client->RegisterPacketHandler(GamePacket::Type::String_Message, &clientReceiver);
	bool ok = client->Connect(127, 0, 0, 1, NetworkBase::GetDefaultPort());
	if (!ok) {
		return;
	}

	while (true) {
		client->UpdateClient();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void testNetworking() {
	NetworkBase::Initialise();
	TestPacketReceiver serverReceiver("Server");
	TestPacketReceiver clientReceiver("Client");

	int port = NetworkBase::GetDefaultPort();
	GameServer* server = new GameServer(port, 1);
	GameClient* client = new GameClient();

	server->RegisterPacketHandler(GamePacket::Type::String_Message, &serverReceiver);
	client->RegisterPacketHandler(GamePacket::Type::String_Message, &clientReceiver);

	bool connected = client->Connect(127, 0, 0, 1, port);

	for (int i = 0; i < 100; i++) {
		StringPacket p("Hello world!");
		server->SendGlobalPacket(p);

		p = StringPacket("Hello back!");
		client->SendPacket(p);

		server->UpdateServer();
		client->UpdateClient();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	NetworkBase::Destroy();
}

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead.

This time, we've added some extra functionality to the window class - we can
hide or show the

*/
int main(int argc, char** argv) {
	Cli cli(argc, argv);

	//testStateMachine();
	//testBehaviourTree();
	WindowInitialisation initInfo;
	initInfo.width		= 1280;
	initInfo.height		= 720;
	initInfo.windowTitle = "CSC8503 Game technology!";

	//if (argc >= 2) {
	//	if (argv[1][0] == 's') {
	//		testServer();
	//		return 0;
	//	}
	//	if (argv[1][0] == 'c') {
	//		testClient();
	//		return 0;
	//	}
	//}

	////testNetworking();
	//return 0;

	Window*w = Window::CreateGameWindow(initInfo);

/*	testPushdownAutomata(w);
	return 0*/;

	if (!w->HasInitialised()) {
		return -1;
	}

	//w->ShowOSPointer(false);
	//w->LockMouseToWindow(true);

	//TutorialGame* g = new TutorialGame();
	NetworkedGame* g = new NetworkedGame(cli);
	w->GetTimer().GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::NEXT)) {
			w->ShowConsole(false);
		}

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		g->UpdateGame(dt);
		DisplayPathfinding();
	}
	Window::DestroyGameWindow();
}
