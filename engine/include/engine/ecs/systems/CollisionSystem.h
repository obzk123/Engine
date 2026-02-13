#pragma once
#include "engine/ecs/Registry.h"

namespace eng::ecs::systems {

/// Sistema de colisiones AABB.
/// Corre en FixedUpdate DESPUES de MovementSystem.
///
/// 1. Construye un spatial hash grid con todas las entidades que tienen
///    Transform2D + BoxCollision.
/// 2. Para cada entidad movil (tiene Velocity2D + BoxCollision):
///    a. Chequea colision contra tiles solidos del tilemap.
///    b. Chequea colision contra entidades solidas cercanas (via spatial grid).
///    c. Resuelve penetracion por el eje de menor solapamiento (minimum penetration).
void CollisionSystem(Registry& reg, float dt);

} // namespace eng::ecs::systems
