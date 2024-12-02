#pragma once
#include "NetworkBase.h"
#include <stdint.h>
#include <thread>
#include <atomic>

namespace NCL {
	namespace CSC8503 {
		class GameObject;
		class GameClient : public NetworkBase {
		public:
			GameClient();
			~GameClient();

			bool Connect(uint32_t address, int portNum);
			bool Connect(uint8_t a, uint8_t b, uint8_t c, uint8_t d, int portNum) {
				return Connect((d << 24) | (c << 16) | (b << 8) | a, portNum);
			}

			void SendPacket(GamePacket&  payload);

			void UpdateClient();

			bool isConnected() const {
				return connected;
			}
		protected:	
			_ENetPeer*	netPeer;

			bool connected;
		};
	}
}

