#include "engine/ecs/systems/RenderSystem.h"
#include "engine/ecs/systems/TilemapRenderSystem.h"
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

    auto view = reg.view<Transform2D, RenderQuad>();
    for (auto [e, t, rq] : view) {
        glm::vec2 renderPos = lerpVec2(t.prevPosition, t.position, alpha);
        r.submitQuad({renderPos.x, renderPos.y}, rq.w, rq.h, rq.color);
    }
    r.flush();

    // ── Tilemap (entre quads de color y sprites) ──
    TilemapRenderSystem(reg, alpha, cam, w, h);

    // ── Capa 1: sprites texturizados ──
    auto spriteView = reg.view<Transform2D, Sprite>();
    for (auto [e, t, spr] : spriteView) {
        glm::vec2 renderPos = lerpVec2(t.prevPosition, t.position, alpha);
        uint32_t glId = ctx.textures->glId(spr.texture);

        eng::Rect uv = spr.uvRect;
        if (spr.flipX) {
            // Invertir horizontalmente: mover x al borde derecho y hacer w negativo.
            // submitTexturedQuad usa u0=uv.x, u1=uv.x+uv.w, asi que con w negativo
            // u0 > u1 y la GPU dibuja la textura espejada.
            uv.x = uv.x + uv.w;
            uv.w = -uv.w;
        }

        r.submitTexturedQuad(renderPos, spr.width, spr.height,
                             glId, uv, spr.tint);
    }

    r.flush();
}

} // namespace eng::ecs::systems
