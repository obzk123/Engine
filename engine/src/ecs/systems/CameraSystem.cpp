#include "engine/ecs/systems/CameraSystem.h"
#include "engine/ecs/Components.h"
#include "engine/render/Renderer2D.h"
#include "engine/Math.h"

#include <SDL.h>
#include <algorithm>
#include <cmath>

namespace eng::ecs::systems {

void CameraSystem(Registry& reg, float alpha) {
    auto& ctx = reg.ctx();
    auto& r   = *ctx.renderer;
    int screenW = 1, screenH = 1;
    SDL_GetWindowSize(ctx.window, &screenW, &screenH);
    constexpr float kPPU = 64.0f;
    // Buscar el player para obtener su posicion interpolada como target
    glm::vec2 target{0.0f, 0.0f};
    auto pv = reg.view<PlayerTag, Transform2D>();
    if (pv.valid()) {
        auto [e, tag, t] = *pv.begin();
        (void)e; (void)tag;
        target = eng::lerpVec2(t.prevPosition, t.position, alpha);
    }

    // Actualizar la camara (normalmente solo hay una)
    auto camView = reg.view<Camera>();
    if (!camView.valid()) return;

    auto [camE, cam] = *camView.begin();
    (void)camE;

    // ── Smooth follow ──
    // Exponential damping: frame-rate independent.
    constexpr float dt = 1.0f / 60.0f;
    float t = 1.0f - std::exp(-cam.smoothSpeed * dt);
    cam.position.x = eng::lerp(cam.position.x, target.x, t);
    cam.position.y = eng::lerp(cam.position.y, target.y, t);

    // ── Clamping a limites del mapa ──
    float halfViewW = (static_cast<float>(screenW) / kPPU) * 0.5f;
    float halfViewH = (static_cast<float>(screenH) / kPPU) * 0.5f;

    float minCamX = cam.mapLeft   + halfViewW;
    float maxCamX = cam.mapRight  - halfViewW;
    float minCamY = cam.mapTop    + halfViewH;
    float maxCamY = cam.mapBottom - halfViewH;

    if (minCamX > maxCamX)
        cam.position.x = (cam.mapLeft + cam.mapRight) * 0.5f;
    else
        cam.position.x = std::clamp(cam.position.x, minCamX, maxCamX);

    if (minCamY > maxCamY)
        cam.position.y = (cam.mapTop + cam.mapBottom) * 0.5f;
    else
        cam.position.y = std::clamp(cam.position.y, minCamY, maxCamY);

    r.setCamera(cam.position, kPPU);
}

} // namespace eng::ecs::systems
