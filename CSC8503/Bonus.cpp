#include "Bonus.h"

#include "NetworkedGame.h"
#include "NetworkPlayer.h"

namespace NCL::CSC8503 {
    void Bonus::OnCollisionBegin(GameObject *other)
    {
        if (!game->isServer()) {
            return;
        }

        if (auto player = dynamic_cast<NetworkPlayer*>(other)) {
            auto id = player->getClientID();
            game->setPlayerScore(id, game->getPlayerScore(id) + value);
            game->getServer()->sendPlayerList();
            game->removeObject(this);
        }
    }
    void Bonus::OnUpdate(float dt)
    {
        float deltaYaw = spinSpeed * dt;
        GetTransform().SetOrientation(
			GetTransform().GetOrientation() *
			Quaternion::EulerAnglesToQuaternion(0, deltaYaw, 0)
		);
    }
};
