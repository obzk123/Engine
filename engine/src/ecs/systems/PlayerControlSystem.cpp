#include "engine/ecs/systems/PlayerControlSystem.h"
#include "engine/ecs/Components.h"
#include "engine/Input.h"
#include "engine/Actions.h"
#include <cmath>

namespace eng::ecs::systems {

void PlayerControlSystem(Registry& reg, float /*fixedDt*/) {
    auto view = reg.view<PlayerTag, Velocity2D>();

    const float speed = 3.0f;

    for (auto [e, tag, vel] : view) {
        (void)tag;

        float x = 0.0f;
        float y = 0.0f;

        if (eng::Input::actionDown(eng::Action::MoveLeft))  x -= 1.0f;
        if (eng::Input::actionDown(eng::Action::MoveRight)) x += 1.0f;
        if (eng::Input::actionDown(eng::Action::MoveUp))    y -= 1.0f;
        if (eng::Input::actionDown(eng::Action::MoveDown))  y += 1.0f;

        // Normalizar diagonal
        const float len2 = x*x + y*y;
        if (len2 > 0.0f) {
            const float invLen = 1.0f / SDL_sqrtf(len2);
            x *= invLen;
            y *= invLen;
        }

        vel.velocity = { x * speed, y * speed };

        // ── Cambiar clip de animacion y flipX segun direccion ──
        // Clips: 0=idle, 1=walk_down, 2=walk_up, 3=walk_side
        if (reg.has<SpriteAnimator>(e) && reg.has<Sprite>(e)) {
            auto& animator = reg.get<SpriteAnimator>(e);
            auto& sprite   = reg.get<Sprite>(e);
            int newClip = 0; // idle por defecto

            if (len2 > 0.0f) {
                // Hay movimiento — elegir clip segun la direccion predominante
                if (std::abs(y) >= std::abs(x)) {
                    // Movimiento vertical predominante
                    newClip = (y > 0.0f) ? 1 : 2;  // down : up
                } else {
                    // Movimiento horizontal predominante
                    newClip = 3; // walk_side
                }
            }

            // FlipX: espejado horizontal cuando va a la izquierda
            if (x < 0.0f)      sprite.flipX = true;
            else if (x > 0.0f) sprite.flipX = false;
            // Si x == 0 (solo vertical), mantiene el ultimo flip

            if (newClip != animator.currentClip) {
                animator.currentClip  = newClip;
                animator.currentFrame = 0;
                animator.timer        = 0.0f;
                animator.playing      = true;
            }
        }
    }
}

} // namespace eng::ecs::systems
