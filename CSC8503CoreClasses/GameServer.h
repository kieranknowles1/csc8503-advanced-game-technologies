#pragma once
#include "NetworkBase.h"

namespace NCL {
	namespace CSC8503 {
		class GameWorld;
		class GameServer : public NetworkBase {
		public:
			GameServer(int onPort, int maxClients);
			~GameServer();

			bool Initialise();
			void Shutdown();

			void SetGameWorld(GameWorld &g);

			bool SendGlobalPacket(GamePacket::Type msgID);
			bool SendGlobalPacket(GamePacket& packet);

			virtual void UpdateServer();

			int getClientCount() const {
				return clientCount;
			}

		protected:
			int			port;
			int			clientMax;
			int			clientCount;
			GameWorld*	gameWorld;

			int incomingDataRate;
			int outgoingDataRate;
		};
	}
}
