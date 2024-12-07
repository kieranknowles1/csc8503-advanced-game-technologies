#include "NetworkObject.h"
#include "./enet/enet.h"
using namespace NCL;
using namespace CSC8503;

NetworkObject::NetworkObject(GameObject& o, Id id) : object(o)	{
	deltaErrors = 0;
	fullErrors  = 0;
	networkID   = id;
}

NetworkObject::~NetworkObject()	{
}

bool NetworkObject::ReadPacket(GamePacket& p) {
	switch (p.type) {
	case GamePacket::Type::Delta_State:
		return ReadDeltaPacket((DeltaPacket&)p);
	case GamePacket::Type::Full_State:
		return ReadFullPacket((FullPacket&)p);
	default: // We don't care about this packet type
		return false;
	}
}

bool NetworkObject::WritePacket(GamePacket** p, bool deltaFrame, int stateID) {
	if (deltaFrame) {
		// Fall back to a full packet if delta fails
		if (!WriteDeltaPacket(p, stateID)) {
			return WriteFullPacket(p);
		}
		return true;
	}
	// Send a full snapshot
	return WriteFullPacket(p);
}
//Client objects recieve these packets
bool NetworkObject::ReadDeltaPacket(DeltaPacket &p) {
	// This is relative to the wrong FullPacket
	if (p.fullID != lastFullState.stateID) {
		std::cout << "DeltaPacket for object " << networkID << " is out of date!\n";
		return false;
	}
	UpdateStateHistory(p.fullID);

	Vector3 fullPos = lastFullState.position;
	Quaternion fullOr = lastFullState.orientation;

	fullPos.x += p.pos[0] / DeltaPositionFactor;
	fullPos.y += p.pos[1] / DeltaPositionFactor;
	fullPos.z += p.pos[2] / DeltaPositionFactor;

	fullOr.x += ((float)p.orientation[0] / DeltaOrientationFactor);
	fullOr.y += ((float)p.orientation[1] / DeltaOrientationFactor);
	fullOr.z += ((float)p.orientation[2] / DeltaOrientationFactor);
	fullOr.w += ((float)p.orientation[3] / DeltaOrientationFactor);

	object.GetTransform().SetPosition(fullPos);
	object.GetTransform().SetOrientation(fullOr);
	return true;
}

bool NetworkObject::ReadFullPacket(FullPacket &p) {
	// This was an old packet that we've gone past
	if (p.fullState.stateID < lastFullState.stateID) {
		return false;
	}
	lastFullState = p.fullState;
	object.GetTransform().SetPosition(lastFullState.position);
	object.GetTransform().SetOrientation(lastFullState.orientation);
	return true;
}

bool NetworkObject::WriteDeltaPacket(GamePacket**p, int stateID) {
	NetworkState state;
	if (!GetNetworkState(stateID, state)) {
		return false; // Nothing to delta against
	}

	DeltaPacket* dp = new DeltaPacket();
	dp->fullID = stateID;
	dp->objectID = networkID;
	Vector3 currentPos = object.GetTransform().GetPosition();
	Quaternion currentOr = object.GetTransform().GetOrientation();

	// Only write out the differences
	currentPos -= state.position;
	currentOr -= state.orientation;

	// NOTE: We write deltas as integers to save bandwidth, we need occasional full updates to correct errors
	// TODO: Detect overflows and fail to send the delta packet
	dp->pos[0] = (DeltaPacket::PositionType)(currentPos.x * DeltaPositionFactor);
	dp->pos[1] = (DeltaPacket::PositionType)(currentPos.y * DeltaPositionFactor);
	dp->pos[2] = (DeltaPacket::PositionType)(currentPos.z * DeltaPositionFactor);

	dp->orientation[0] = (char)(currentOr.x * DeltaOrientationFactor);
	dp->orientation[1] = (char)(currentOr.y * DeltaOrientationFactor);
	dp->orientation[2] = (char)(currentOr.z * DeltaOrientationFactor);
	dp->orientation[3] = (char)(currentOr.w * DeltaOrientationFactor);
	*p = dp;
	lastDeltaState = createNetworkState(stateID);
	return true;
}

NetworkState NetworkObject::createNetworkState(int id) {
	NetworkState state;
	state.position = object.GetTransform().GetPosition();
	state.orientation = object.GetTransform().GetOrientation();
	state.stateID = id;
	return state;
}

bool NetworkObject::WriteFullPacket(GamePacket**p) {
	FullPacket* fp = new FullPacket();
	lastFullState = createNetworkState(lastFullState.stateID + 1);
	fp->fullState = lastFullState;
	fp->objectID = networkID;
	stateHistory.push_back(lastFullState);
	*p = fp;
	return true;
}

NetworkState& NetworkObject::GetLatestNetworkState() {
	return lastFullState;
}

bool NetworkObject::GetNetworkState(int stateID, NetworkState& state) {
	auto it = std::find_if(stateHistory.begin(), stateHistory.end(), [&](NetworkState& s) {
		return s.stateID == stateID;
	});
	if (it == stateHistory.end()) {
		return false;
	}
	state = *it;
	return true;
}

// Drop states that are older than the given ID
void NetworkObject::UpdateStateHistory(int minID) {
	std::erase_if(stateHistory, [&](NetworkState& state) {
		return state.stateID < minID;
	});
}

int NetworkObject::getDeltaError(const NetworkState& from) const {
	// We're a new object, so we should always send a full state
	if (from.stateID == 0) {
		return std::numeric_limits<int>::max();
	}

	Vector3 posError = object.GetTransform().GetPosition() - from.position;
	Quaternion orError = object.GetTransform().GetOrientation() - from.orientation;

	int posFactor = Vector::Length(posError) * DeltaPositionFactor;
	int orFactor = Vector::Length(Vector3(orError.x, orError.y, orError.z)) * DeltaOrientationFactor * 128;

	return posFactor + orFactor;
}
