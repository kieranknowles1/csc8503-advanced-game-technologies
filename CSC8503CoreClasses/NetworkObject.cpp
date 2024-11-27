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
	case BasicNetworkMessages::Delta_State:
		return ReadDeltaPacket((DeltaPacket&)p);
	case BasicNetworkMessages::Full_State:
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
	}
	// Send a full snapshot
	return WriteFullPacket(p);
}
//Client objects recieve these packets
bool NetworkObject::ReadDeltaPacket(DeltaPacket &p) {
	// This is relative to the wrong FullPacket
	if (p.fullID != lastFullState.stateID) {
		return false;
	}
	UpdateStateHistory(p.fullID);

	Vector3 fullPos = lastFullState.position;
	Quaternion fullOr = lastFullState.orientation;

	fullPos.x += p.pos[0];
	fullPos.y += p.pos[1];
	fullPos.z += p.pos[2];

	fullOr.x += ((float)p.orientation[0] / 127.0f);
	fullOr.y += ((float)p.orientation[1] / 127.0f);
	fullOr.z += ((float)p.orientation[2] / 127.0f);
	fullOr.w += ((float)p.orientation[3] / 127.0f);

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
	dp->pos[0] = (char)(currentPos.x);
	dp->pos[1] = (char)(currentPos.y);
	dp->pos[2] = (char)(currentPos.z);

	dp->orientation[0] = (char)(currentOr.x * 127.0f);
	dp->orientation[1] = (char)(currentOr.y * 127.0f);
	dp->orientation[2] = (char)(currentOr.z * 127.0f);
	dp->orientation[3] = (char)(currentOr.w * 127.0f);
	*p = dp;
	return true;
}

bool NetworkObject::WriteFullPacket(GamePacket**p) {
	FullPacket* fp = new FullPacket();
	fp->objectID = networkID;
	fp->fullState.position = object.GetTransform().GetPosition();
	fp->fullState.orientation = object.GetTransform().GetOrientation();
	fp->fullState.stateID = lastFullState.stateID;
	lastFullState.stateID++;
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