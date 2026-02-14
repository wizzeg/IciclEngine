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