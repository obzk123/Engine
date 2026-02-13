#include "engine/ecs/systems/RenderSystem.h"
#include "engine/ecs/systems/TilemapRenderSystem.h"
#include "engine/ecs/Components.h"
#include "engine/render/Renderer2D.h"
#include "engine/render/TextureManager.h"
#include "engine/Math.h"

#include <SDL.h>
#include <vector>
#include <algorithm>

namespace eng::ecs::systems {

using eng::lerp;
using eng::lerpVec2;

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
    constexpr float kPPU = 64.0f;
    r.setCamera(cam, kPPU);

    auto view = reg.view<Transform2D, RenderQuad>();
    for (auto [e, t, rq] : view) {
        glm::vec2 renderPos = lerpVec2(t.prevPosition, t.position, alpha);
        r.submitQuad({renderPos.x, renderPos.y}, rq.w, rq.h, rq.color);
    }
    r.flush();

    // ── Tilemap (entre quads de color y sprites) ──
    TilemapRenderSystem(reg, alpha, cam, w, h, kPPU);

    // ── Capa 1: sprites texturizados (Y-sorted para profundidad) ──
    // Recolectar sprites con su sortY (borde inferior = pies).
    // Entidades con Y mayor (mas abajo en pantalla) se dibujan despues (encima).
    struct SpriteEntry {
        float      sortY;      // Y del borde inferior del sprite (los "pies")
        glm::vec2  renderPos;
        uint32_t   glId;
        eng::Rect  uv;
        float      width, height;
        eng::ecs::Color4 tint;
    };
    std::vector<SpriteEntry> spriteEntries;

    auto spriteView = reg.view<Transform2D, Sprite>();
    for (auto [e, t, spr] : spriteView) {
        glm::vec2 renderPos = lerpVec2(t.prevPosition, t.position, alpha);
        uint32_t glId = ctx.textures->glId(spr.texture);

        eng::Rect uv = spr.uvRect;
        if (spr.flipX) {
            uv.x = uv.x + uv.w;
            uv.w = -uv.w;
        }

        // sortY = borde inferior del sprite (centro + mitad de altura)
        float sortY = renderPos.y + spr.height * 0.5f;

        spriteEntries.push_back({sortY, renderPos, glId, uv,
                                 spr.width, spr.height, spr.tint});
    }

    // Ordenar: menor Y primero (mas lejos), mayor Y despues (mas cerca, se dibuja encima)
    std::sort(spriteEntries.begin(), spriteEntries.end(),
              [](const SpriteEntry& a, const SpriteEntry& b) {
                  return a.sortY < b.sortY;
              });

    for (const auto& se : spriteEntries) {
        r.submitTexturedQuad(se.renderPos, se.width, se.height,
                             se.glId, se.uv, se.tint);
    }

    r.flush();
}

} // namespace eng::ecs::systems
