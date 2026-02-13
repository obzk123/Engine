#include "engine/ecs/systems/CollisionSystem.h"
#include "engine/ecs/Components.h"

#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace eng::ecs::systems {

// ────────────────────────────────────────────────────────────────
// AABB helpers
// ────────────────────────────────────────────────────────────────

struct AABB {
    float left, top, right, bottom;
};

/// Calcula el AABB en world space para una entidad con Transform2D + BoxCollision.
static AABB makeAABB(const eng::ecs::Transform2D& t, const eng::ecs::BoxCollision& b) {
    float cx = t.position.x + b.offsetX;
    float cy = t.position.y + b.offsetY;
    float hw = b.width  * 0.5f;
    float hh = b.height * 0.5f;
    return { cx - hw, cy - hh, cx + hw, cy + hh };
}

/// Retorna true si dos AABBs se solapan.
static bool overlaps(const AABB& a, const AABB& b) {
    return a.left < b.right && a.right > b.left &&
           a.top < b.bottom && a.bottom > b.top;
}

// ────────────────────────────────────────────────────────────────
// Spatial Hash Grid
// ────────────────────────────────────────────────────────────────

/// Tamano de cada celda del spatial grid en world units.
/// 2x2 es un buen balance para entidades de ~1 world unit.
static constexpr float kCellSize = 2.0f;

/// Hash key para una celda (col, row) del spatial grid.
struct CellKey {
    int32_t cx, cy;
    bool operator==(const CellKey& o) const { return cx == o.cx && cy == o.cy; }
};

struct CellKeyHash {
    size_t operator()(const CellKey& k) const {
        // Hash simple pero efectivo para pares de ints
        auto h1 = std::hash<int32_t>{}(k.cx);
        auto h2 = std::hash<int32_t>{}(k.cy);
        return h1 ^ (h2 * 2654435761u);
    }
};

using SpatialGrid = std::unordered_map<CellKey, std::vector<Entity>, CellKeyHash>;

/// Inserta una entidad en todas las celdas que su AABB toca.
static void insertIntoGrid(SpatialGrid& grid, Entity e, const AABB& box) {
    int minCX = static_cast<int>(std::floor(box.left   / kCellSize));
    int maxCX = static_cast<int>(std::floor(box.right  / kCellSize));
    int minCY = static_cast<int>(std::floor(box.top    / kCellSize));
    int maxCY = static_cast<int>(std::floor(box.bottom / kCellSize));

    for (int cy = minCY; cy <= maxCY; ++cy)
        for (int cx = minCX; cx <= maxCX; ++cx)
            grid[{cx, cy}].push_back(e);
}

/// Retorna todas las entidades en las celdas que toca el AABB dado.
/// Puede contener duplicados — el caller los filtra.
static void queryGrid(const SpatialGrid& grid, const AABB& box,
                      std::vector<Entity>& out) {
    int minCX = static_cast<int>(std::floor(box.left   / kCellSize));
    int maxCX = static_cast<int>(std::floor(box.right  / kCellSize));
    int minCY = static_cast<int>(std::floor(box.top    / kCellSize));
    int maxCY = static_cast<int>(std::floor(box.bottom / kCellSize));

    for (int cy = minCY; cy <= maxCY; ++cy) {
        for (int cx = minCX; cx <= maxCX; ++cx) {
            auto it = grid.find({cx, cy});
            if (it != grid.end()) {
                for (Entity e : it->second)
                    out.push_back(e);
            }
        }
    }
}

// ────────────────────────────────────────────────────────────────
// Tile collision
// ────────────────────────────────────────────────────────────────

/// Resuelve colision de un AABB movil contra los tiles solidos del tilemap.
/// Modifica pos directamente (minimum penetration por eje).
static void resolveTileCollisions(glm::vec2& pos,
                                  const eng::ecs::BoxCollision& box,
                                  const eng::ecs::Tilemap& tm,
                                  const eng::ecs::TileCollisionLayer& tcl,
                                  const glm::vec2& mapPos) {
    // Calcular AABB actual del movil
    float cx = pos.x + box.offsetX;
    float cy = pos.y + box.offsetY;
    float hw = box.width  * 0.5f;
    float hh = box.height * 0.5f;

    float left   = cx - hw;
    float right  = cx + hw;
    float top    = cy - hh;
    float bottom = cy + hh;

    // Rango de tiles que toca el AABB (en coordenadas del tilemap)
    int colMin = static_cast<int>(std::floor(left   - mapPos.x));
    int colMax = static_cast<int>(std::floor(right  - mapPos.x));
    int rowMin = static_cast<int>(std::floor(top    - mapPos.y));
    int rowMax = static_cast<int>(std::floor(bottom - mapPos.y));

    colMin = std::max(0, colMin);
    colMax = std::min(tm.width  - 1, colMax);
    rowMin = std::max(0, rowMin);
    rowMax = std::min(tm.height - 1, rowMax);

    for (int row = rowMin; row <= rowMax; ++row) {
        for (int col = colMin; col <= colMax; ++col) {
            if (!tcl.solid[row * tm.width + col]) continue;

            // AABB del tile en world space
            float tileL = mapPos.x + static_cast<float>(col);
            float tileR = tileL + 1.0f;
            float tileT = mapPos.y + static_cast<float>(row);
            float tileB = tileT + 1.0f;

            // Recalcular AABB del movil (pudo haber cambiado por resoluciones previas)
            cx = pos.x + box.offsetX;
            cy = pos.y + box.offsetY;
            left   = cx - hw;
            right  = cx + hw;
            top    = cy - hh;
            bottom = cy + hh;

            // Chequear overlap
            if (left >= tileR || right <= tileL || top >= tileB || bottom <= tileT)
                continue;

            // Calcular penetracion en cada eje
            float overlapL = right  - tileL;   // penetracion desde la izquierda del tile
            float overlapR = tileR  - left;    // penetracion desde la derecha del tile
            float overlapT = bottom - tileT;   // penetracion desde arriba del tile
            float overlapB = tileB  - top;     // penetracion desde abajo del tile

            // Encontrar el eje de menor penetracion
            float minOverlap = overlapL;
            float resolveX = -overlapL;
            float resolveY = 0.0f;

            if (overlapR < minOverlap) {
                minOverlap = overlapR;
                resolveX = overlapR;
                resolveY = 0.0f;
            }
            if (overlapT < minOverlap) {
                minOverlap = overlapT;
                resolveX = 0.0f;
                resolveY = -overlapT;
            }
            if (overlapB < minOverlap) {
                resolveX = 0.0f;
                resolveY = overlapB;
            }

            pos.x += resolveX;
            pos.y += resolveY;
        }
    }
}

// ────────────────────────────────────────────────────────────────
// CollisionSystem
// ────────────────────────────────────────────────────────────────

void CollisionSystem(Registry& reg, float /*dt*/) {
    // ── 1. Construir spatial grid con todas las entidades con collider ──
    SpatialGrid grid;

    auto allColliders = reg.view<Transform2D, BoxCollision>();
    for (auto [e, t, b] : allColliders) {
        if (!b.isSolid) continue;
        AABB box = makeAABB(t, b);
        insertIntoGrid(grid, e, box);
    }

    // ── 2. Buscar tilemap y tile collision layer ──
    // (puede haber 0 o 1 tilemap en la escena)
    Transform2D* tmTransform = nullptr;
    Tilemap*     tmPtr = nullptr;
    TileCollisionLayer* tclPtr = nullptr;
    {
        auto tmView = reg.view<Transform2D, Tilemap, TileCollisionLayer>();
        if (tmView.valid()) {
            auto [e, t, tm, tcl] = *tmView.begin();
            (void)e;
            tmTransform = &t;
            tmPtr       = &tm;
            tclPtr      = &tcl;
        }
    }

    // ── 3. Para cada entidad movil, resolver colisiones ──
    auto movers = reg.view<Transform2D, Velocity2D, BoxCollision>();
    std::vector<Entity> nearby;

    for (auto [e, t, v, b] : movers) {
        if (!b.isSolid) continue;

        // 3a. Colision contra tiles solidos
        if (tmPtr && tclPtr && tmTransform) {
            resolveTileCollisions(t.position, b, *tmPtr, *tclPtr,
                                  tmTransform->position);
        }

        // 3b. Colision contra entidades solidas cercanas
        AABB myBox = makeAABB(t, b);
        nearby.clear();
        queryGrid(grid, myBox, nearby);

        for (Entity other : nearby) {
            if (other == e) continue;

            // Solo contra entidades solidas
            if (!reg.has<BoxCollision>(other)) continue;
            auto& otherB = reg.get<BoxCollision>(other);
            if (!otherB.isSolid) continue;

            auto& otherT = reg.get<Transform2D>(other);
            AABB otherBox = makeAABB(otherT, otherB);

            // Recalcular mi AABB (pudo cambiar por resoluciones previas)
            myBox = makeAABB(t, b);

            if (!overlaps(myBox, otherBox)) continue;

            // Minimum penetration resolution
            float overlapL = myBox.right  - otherBox.left;
            float overlapR = otherBox.right  - myBox.left;
            float overlapT = myBox.bottom - otherBox.top;
            float overlapB = otherBox.bottom - myBox.top;

            float minOverlap = overlapL;
            float resolveX = -overlapL;
            float resolveY = 0.0f;

            if (overlapR < minOverlap) {
                minOverlap = overlapR;
                resolveX = overlapR;
                resolveY = 0.0f;
            }
            if (overlapT < minOverlap) {
                minOverlap = overlapT;
                resolveX = 0.0f;
                resolveY = -overlapT;
            }
            if (overlapB < minOverlap) {
                resolveX = 0.0f;
                resolveY = overlapB;
            }

            t.position.x += resolveX;
            t.position.y += resolveY;
        }
    }
}

} // namespace eng::ecs::systems
