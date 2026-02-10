#include "engine/ecs/systems/AnimationSystem.h"
#include "engine/ecs/Components.h"

namespace eng::ecs::systems {

void AnimationSystem(Registry& reg, float dt) {
    // Iterar todas las entidades que tienen SpriteAnimator Y Sprite.
    // El sistema avanza el timer, cambia de frame, y actualiza el uvRect del Sprite.
    auto view = reg.view<SpriteAnimator, Sprite>();

    for (auto [e, animator, sprite] : view) {
        if (!animator.playing) continue;
        if (animator.clips.empty()) continue;

        auto& clip = animator.clips[animator.currentClip];
        if (clip.frames.empty()) continue;

        // Avanzar timer
        animator.timer += dt;

        // Cambiar de frame si paso suficiente tiempo
        while (animator.timer >= clip.frameDuration) {
            animator.timer -= clip.frameDuration;
            animator.currentFrame++;

            if (animator.currentFrame >= (int)clip.frames.size()) {
                if (clip.loop) {
                    animator.currentFrame = 0;
                } else {
                    animator.currentFrame = (int)clip.frames.size() - 1;
                    animator.playing = false;
                    break;
                }
            }
        }

        // Actualizar el UV del sprite con el frame actual
        sprite.uvRect = clip.frames[animator.currentFrame];
    }
}

} // namespace eng::ecs::systems
