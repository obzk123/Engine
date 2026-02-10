#include "engine/ecs/systems/MovementSystem.h"
#include "engine/ecs/Components.h"

namespace eng::ecs::systems {

void MovementSystem(Registry& reg, float dt) {
    auto view = reg.view<Transform2D, Velocity2D>();
    for (auto [e, t, v] : view) {
        t.prevPosition = t.position;
        t.position.x += v.velocity.x * dt;
        t.position.y += v.velocity.y * dt;
    }
}

} // namespace eng::ecs::systems
