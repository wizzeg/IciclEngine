#pragma once
#include <engine/editor/systems_registry.h>
#include <engine/game/systems.h>

REGISTER_SYSTEM(MoveSystem)
REGISTER_SYSTEM(TransformCalculationSystem)
REGISTER_SYSTEM(AABBWriteSpatialPartitionSystem)
REGISTER_SYSTEM(AABBReadSpatialPartitionSystem)