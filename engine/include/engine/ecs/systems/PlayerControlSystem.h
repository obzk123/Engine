#pragma once
#include "engine/ecs/Registry.h"

namespace eng::ecs::systems {
    void PlayerControlSystem(Registry& reg, float fixedDt);
}
