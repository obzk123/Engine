#include "engine/ecs/systems/RenderSystem.h"
#include "engine/ecs/Components.h"
#include "engine/render/Renderer2D.h"
#include "engine/render/TextureManager.h"

#include <SDL.h>

namespace eng::ecs::systems {

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static glm::vec2 lerpVec2(const glm::vec2& a, const glm::vec2& b, float t) {
    return { lerp(a.x, b.x, t), lerp(a.y, b.y, t) };
}

void RenderSystem(Registry& reg, float alpha) {
    // Obtener window y renderer del contexto del registry (sin globals)
    auto& ctx = reg.ctx();
    int w = 1, h = 1;
    SDL_GetWindowSize(ctx.window, &w, &h);

    auto& r = *ctx.renderer;
    r.beginFrame(w, h);

    // Camara: centrada en el player si existe
    glm::vec2 cam{0,0};
    {
        auto pv = reg.view<PlayerTag, Transform2D>();
        for (auto [e, tag, t] : pv) {
            (void)e; (void)tag;
            cam = lerpVec2(t.prevPosition, t.position, alpha);
            break;
        }
    }
    r.setCamera(cam, 64.0f);

    // ── Capa 0: grilla de debug y marcador de origen ──
    r.submitQuad({0.0f, 0.0f}, 0.2f, 0.2f, {1.0f, 0.0f, 0.0f, 1.0f});
    for (int i = -10; i <= 10; ++i) {
        r.submitQuad({(float)i, 0.0f}, 0.03f, 25.0f, {0.2f, 0.2f, 0.2f, 0.6f});
        r.submitQuad({0.0f, (float)i}, 25.0f, 0.03f, {0.2f, 0.2f, 0.2f, 0.6f});
    }

    // Dibujar todos los RenderQuad (quads de color solido)
    auto view = reg.view<Transform2D, RenderQuad>();
    for (auto [e, t, rq] : view) {
        glm::vec2 renderPos = lerpVec2(t.prevPosition, t.position, alpha);
        r.submitQuad({renderPos.x, renderPos.y}, rq.w, rq.h, rq.color);
    }

    // Flush quads de color antes de dibujar sprites texturizados.
    // Esto evita artefactos de blending entre la textura dummy blanca
    // (usada por submitQuad) y las texturas reales en el mismo batch.
    r.flush();

    // ── Capa 1: sprites texturizados ──
    auto spriteView = reg.view<Transform2D, Sprite>();
    for (auto [e, t, spr] : spriteView) {
        glm::vec2 renderPos = lerpVec2(t.prevPosition, t.position, alpha);
        uint32_t glId = ctx.textures->glId(spr.texture);
        r.submitTexturedQuad(renderPos, spr.width, spr.height,
                             glId, spr.uvRect, spr.tint);
    }

    r.flush();
}

} // namespace eng::ecs::systems
