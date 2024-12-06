#pragma once
#include "GameObject.h"
#include "NetworkBase.h"
#include "NetworkState.h"

namespace NCL::CSC8503 {
	class GameObject;

	struct FullPacket : public GamePacket {
		int		objectID = -1;
		// The full state of the object
		NetworkState fullState;

		FullPacket() : GamePacket(Type::Full_State) {
			size = sizeof(FullPacket) - sizeof(GamePacket);
		}
	};

	// Divisor for delta position values, converts from fixed point to float
	// Divide by this when reading, multiply by this when writing
	// ~1mm resolution
	const constexpr float DeltaPositionFactor = 1024;
	const constexpr float DeltaOrientationFactor = 127;
	struct DeltaPacket : public GamePacket {
		using PositionType = short;

		// The ID of the last full state
		// Reject this delta if it doesn't match the last full state
		int		fullID		= -1;
		int		objectID	= -1;
		PositionType	pos[3];
		char	orientation[4];

		DeltaPacket() : GamePacket(Type::Delta_State) {
			type = Type::Delta_State;
			size = sizeof(DeltaPacket) - sizeof(GamePacket);
		}
	};

	class NetworkObject		{
	public:
		using Id = unsigned int;
		const static constexpr Id MaxId = std::numeric_limits<Id>::max();

		NetworkObject(GameObject& o, Id id);
		virtual ~NetworkObject();

		Id getId() const {
			return networkID;
		}

		//Called by clients
		virtual bool ReadPacket(GamePacket& p);
		//Called by servers
		virtual bool WritePacket(GamePacket** p, bool deltaFrame, int stateID);

		void UpdateStateHistory(int minID);

		// Get how far the current state is from the last full state
		// Fairly arbitary metric, intended to detect what kind of state to send,
		// if any
		int getDeltaError(const NetworkState& from) const;

		NetworkState& GetLastFullState() {
			return lastFullState;
		}
		NetworkState& GetLastDeltaState() {
			return lastDeltaState;
		}

	protected:
		NetworkState createNetworkState(int id);

		NetworkState& GetLatestNetworkState();

		bool GetNetworkState(int frameID, NetworkState& state);

		virtual bool ReadDeltaPacket(DeltaPacket &p);
		virtual bool ReadFullPacket(FullPacket &p);

		virtual bool WriteDeltaPacket(GamePacket**p, int stateID);
		virtual bool WriteFullPacket(GamePacket**p);

		GameObject& object;

		NetworkState lastFullState;
		NetworkState lastDeltaState;

		std::vector<NetworkState> stateHistory;

		int deltaErrors;
		int fullErrors;

		Id networkID;
	};
}
