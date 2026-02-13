#pragma once

#include "engine/ecs/Components.h"
#include "engine/ecs/Registry.h"

namespace eng::ecs::systems {
    void CameraSystem(Registry& reg, float dt);
}