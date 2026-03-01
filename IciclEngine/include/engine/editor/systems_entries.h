#pragma once
#include <engine/editor/systems_registry.h>
#include <engine/game/systems.h>

REGISTER_SYSTEM(MoveSystem)
REGISTER_SYSTEM(TransformCalculationSystem)
REGISTER_SYSTEM(PhysicsSystem)
REGISTER_SYSTEM(AABBReadSpatialPartitionSystem)
REGISTER_SYSTEM(SpawnCubeSystem)
REGISTER_SYSTEM(ECBCreatorSystem)
REGISTER_SYSTEM(DestroyCubeSystem)
REGISTER_SYSTEM(SyncSystem)
REGISTER_SYSTEM(HeightMapLoadSystem)
REGISTER_SYSTEM(UIDrawGathering)
REGISTER_SYSTEM(MenuSystem)
REGISTER_SYSTEM(ReadCollisionResultSystem)
REGISTER_SYSTEM(PlayerMovementSystem)
REGISTER_SYSTEM(GroundingSystem)
REGISTER_SYSTEM(ButtonSystem)
REGISTER_SYSTEM(ECBEndSystem)
REGISTER_SYSTEM(CollectionSystem)