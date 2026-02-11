#include "engine/ecs/systems/TilemapRenderSystem.h"
#include "engine/ecs/Components.h"
#include "engine/render/Renderer2D.h"
#include "engine/render/TextureManager.h"

#include <algorithm>

namespace eng::ecs::systems {

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static glm::vec2 lerpVec2(const glm::vec2& a, const glm::vec2& b, float t) {
    return { lerp(a.x, b.x, t), lerp(a.y, b.y, t) };
}

void TilemapRenderSystem(Registry& reg, float alpha,
                         glm::vec2 camCenter, int screenW, int screenH) {
    auto& ctx = reg.ctx();
    auto& r   = *ctx.renderer;

    // Calcular area visible en world units
    const float ppu = 64.0f;
    const float worldW = static_cast<float>(screenW) / ppu;
    const float worldH = static_cast<float>(screenH) / ppu;

    const float camLeft   = camCenter.x - worldW * 0.5f;
    const float camRight  = camCenter.x + worldW * 0.5f;
    const float camTop    = camCenter.y - worldH * 0.5f;
    const float camBottom = camCenter.y + worldH * 0.5f;

    auto view = reg.view<Transform2D, Tilemap>();
    for (auto [e, transform, tilemap] : view) {
        (void)e;
        if (tilemap.layers.empty()) continue;

        // Posicion interpolada del mapa (esquina top-left)
        glm::vec2 mapPos = lerpVec2(transform.prevPosition, transform.position, alpha);

        // Rango de tiles visibles (con 1 tile de margen para evitar pop-in)
        int startCol = static_cast<int>(camLeft   - mapPos.x) - 1;
        int endCol   = static_cast<int>(camRight  - mapPos.x) + 2;
        int startRow = static_cast<int>(camTop    - mapPos.y) - 1;
        int endRow   = static_cast<int>(camBottom - mapPos.y) + 2;

        // Clamp a los limites del mapa
        startCol = std::max(0, startCol);
        endCol   = std::min(tilemap.width, endCol);
        startRow = std::max(0, startRow);
        endRow   = std::min(tilemap.height, endRow);

        // GL texture ID del tileset
        uint32_t glTexId = ctx.textures->glId(tilemap.tileset.texture());

        // Ordenar capas por renderOrder (indices, no copias)
        std::vector<int> layerOrder;
        layerOrder.reserve(tilemap.layers.size());
        for (int i = 0; i < static_cast<int>(tilemap.layers.size()); ++i) {
            layerOrder.push_back(i);
        }
        std::sort(layerOrder.begin(), layerOrder.end(),
                  [&](int a, int b) {
            return tilemap.layers[a].renderOrder < tilemap.layers[b].renderOrder;
        });

        // Dibujar cada capa
        for (int layerIdx : layerOrder) {
            const auto& layer = tilemap.layers[layerIdx];

            for (int row = startRow; row < endRow; ++row) {
                for (int col = startCol; col < endCol; ++col) {
                    uint16_t tileId = layer.tiles[row * tilemap.width + col];
                    if (tileId == 0) continue;

                    Rect uv = tilemap.tileset.getTileUV(tileId);

                    // Centro del tile en world coords
                    glm::vec2 center = mapPos + glm::vec2(
                        static_cast<float>(col) + 0.5f,
                        static_cast<float>(row) + 0.5f
                    );

                    r.submitTexturedQuad(center, 1.0f, 1.0f, glTexId, uv);
                }
            }
        }
    }

    r.flush();
}

} // namespace eng::ecs::systems
