#include "engine/ecs/systems/PlayerControlSystem.h"
#include "engine/ecs/Components.h"
#include "engine/Input.h"
#include "engine/Actions.h"

namespace eng::ecs::systems {

void PlayerControlSystem(Registry& reg, float /*fixedDt*/) {
    // Buscar player (simple por ahora): todas las entidades con PlayerTag + Velocity2D
    auto view = reg.view<PlayerTag, Velocity2D>();

    // Movimiento en 2D tipo top-down
    const float speed = 3.0f; // unidades por segundo

    for (auto [e, tag, vel] : view) {
        (void)tag;

        float x = 0.0f;
        float y = 0.0f;

        if (eng::Input::actionDown(eng::Action::MoveLeft))  x -= 1.0f;
        if (eng::Input::actionDown(eng::Action::MoveRight)) x += 1.0f;
        if (eng::Input::actionDown(eng::Action::MoveUp))    y -= 1.0f; // convención: up = -Y (pantalla)
        if (eng::Input::actionDown(eng::Action::MoveDown))  y += 1.0f;

        // Normalizar diagonal para no correr más rápido en diagonal
        const float len2 = x*x + y*y;
        if (len2 > 0.0f) {
            const float invLen = 1.0f / SDL_sqrtf(len2);
            x *= invLen;
            y *= invLen;
        }

        vel.velocity = { x * speed, y * speed };
    }
}

} // namespace eng::ecs::systems
