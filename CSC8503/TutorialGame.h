#include "../NCLCoreClasses/KeyboardMouseController.h"

#pragma once
#include "GameTechRenderer.h"
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#endif
#include "PhysicsSystem.h"

#include "StateGameObject.h"
#include "Resources.h"
#include "NetworkPlayer.h"

namespace NCL {
	namespace CSC8503 {
		class Bonus;
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);

			GameWorld* getWorld() {
				return world;
			}
		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			virtual void ClearWorld();
			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on).
			*/
			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);

			void InitDefaultFloor();

			bool SelectObject();
			void MoveSelectedObject(float dt);
			void DebugObjectMovement();
			void LockedObjectMovement();

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, bool axisAligned = false);

			NetworkPlayer* AddPlayerToWorld(const Vector3& position, int id);
			GameObject* AddEnemyToWorld(const Vector3& position);

			Vector4 generateCatColor();

#ifdef USEVULKAN
			GameTechVulkanRenderer*	renderer;
#else
			GameTechRenderer* renderer;
#endif
			PhysicsSystem*		physics;
			GameWorld*			world;

			KeyboardMouseController controller;

			bool inSelectionMode;

			// Force to apply when right-clicking the selected object, in Newton seconds
			float		forceMagnitude;

			Resources* resources = nullptr;

			GameObject* selectionObject = nullptr;
			GameObject* selectionVisibleObject = nullptr;

			Mesh*	capsuleMesh = nullptr;
			Mesh*	cubeMesh	= nullptr;
			Mesh*	sphereMesh	= nullptr;

			Texture*	basicTex	= nullptr;
			Shader*		basicShader = nullptr;

			//Coursework Meshes
			Mesh*	catMesh		= nullptr;
			Mesh*	kittenMesh	= nullptr;
			Mesh*	enemyMesh	= nullptr;
			Mesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality
			GameObject* lockedObject	= nullptr;
			Vector3 lockedAngle 	= Vector3(180, -25, 0);
			Vector3 lockedOffset		= Vector3(0, 0, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameObject* objClosest = nullptr;

			// Only the server has authority to run RNG
			// TODO: Make this part of the server class
			Rng rng = Rng(0);
		};
	}
}

