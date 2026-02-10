#pragma once
#include "engine/ecs/Registry.h"

namespace eng::ecs::systems {
    void MovementSystem(Registry& reg, float dt);
}
