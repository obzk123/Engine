#include "engine/Engine.h"
#include "engine/Input.h"
#include "engine/Actions.h"
#include "engine/ecs/Components.h"
#include "engine/ecs/SystemScheduler.h"
#include "engine/ecs/systems/InputSystem.h"
#include "engine/ecs/systems/PlayerControlSystem.h"
#include "engine/ecs/systems/MovementSystem.h"
#include "engine/ecs/systems/RenderSystem.h"
#include "engine/ecs/systems/DebugUISystem.h"
#include "engine/ecs/systems/AnimationSystem.h"
#include "engine/ecs/systems/CollisionSystem.h"
#include "engine/ecs/systems/CameraSystem.h"
#include <SDL.h>

// ─────────────────────────────────────────────────────────────
// Demo: mundo Stardew Valley con tilemap, casa, arboles, granja,
// lago, caminos, animales y NPCs.
// ─────────────────────────────────────────────────────────────
// Controles:
//   WASD  - Mover al player
//   P     - Pausar/reanudar simulacion
//   O     - Avanzar un fixed step (cuando esta pausado)

// ── Tile IDs (ver output de tools/build_tileset.py) ──
// Terrain
constexpr uint16_t GRASS = 1;
// constexpr uint16_t PATH_MID = 2;   // center sin bordes
// constexpr uint16_t WATER_MID = 3;  // center sin bordes

// FarmLand 3x3 (tiles 4..12)
constexpr uint16_t FARM_TL = 4,  FARM_T  = 5,  FARM_TR = 6;
constexpr uint16_t FARM_L  = 7,  FARM_C  = 8,  FARM_R  = 9;
constexpr uint16_t FARM_BL = 10, FARM_B  = 11, FARM_BR = 12;

// Path borders 3x6 (tiles 13..30)
// Row 0: TL(13) T(14) TR(15)   Row 1: L(16) C(17) R(18)   Row 2: BL(19) B(20) BR(21)
// Row 3-5: extra transitions
constexpr uint16_t PATH_TL = 13, PATH_T  = 14, PATH_TR = 15;
constexpr uint16_t PATH_L  = 16, PATH_C  = 17, PATH_R  = 18;
constexpr uint16_t PATH_BL = 19, PATH_B  = 20, PATH_BR = 21;

// Water borders 3x6 (tiles 31..48)
constexpr uint16_t WATER_TL = 31, WATER_T  = 32, WATER_TR = 33;
constexpr uint16_t WATER_L  = 34, WATER_C  = 35, WATER_R  = 36;
constexpr uint16_t WATER_BL = 37, WATER_B  = 38, WATER_BR = 39;

// Cliff borders 3x6 (tiles 49..66)
constexpr uint16_t CLIFF_TL = 49, CLIFF_T  = 50, CLIFF_TR = 51;
constexpr uint16_t CLIFF_L  = 52, CLIFF_C  = 53, CLIFF_R  = 54;
constexpr uint16_t CLIFF_BL = 55, CLIFF_B  = 56, CLIFF_BR = 57;

// Decoraciones del Outdoor_Decor_Free (tiles 82..140)
// Mirando la imagen del tileset visualmente:
// Row 5 (tiles 82-96): arbustos, cajones, plantas, tocón
constexpr uint16_t BUSH_S   = 82;   // arbusto chico
constexpr uint16_t BUSH_M   = 83;   // arbusto mediano
constexpr uint16_t BUSH_L   = 84;   // arbusto grande
constexpr uint16_t CRATE1   = 85;   // cajón cerrado
constexpr uint16_t BARREL1  = 86;   // barril
constexpr uint16_t CRATE2   = 87;   // cajón oscuro
constexpr uint16_t BARREL2  = 88;   // barril oscuro
constexpr uint16_t PLANT1   = 89;   // planta/carrot
constexpr uint16_t PLANT2   = 90;   // planta
constexpr uint16_t PLANT3   = 91;   // planta grande
constexpr uint16_t WEED1    = 92;   // hierba
constexpr uint16_t WEED2    = 93;   // hierba 2
constexpr uint16_t WEED3    = 94;   // hierba/arbusto
constexpr uint16_t VINE     = 95;   // enredadera
constexpr uint16_t STUMP    = 96;   // tronco cortado

// Row 6 (tiles 97-112): rocas, piedras, cajones dorados, etc
constexpr uint16_t ROCK_S   = 97;   // roca chica
constexpr uint16_t ROCK_L   = 98;   // roca grande
constexpr uint16_t MUSHROOM = 99;   // hongo
constexpr uint16_t PUMPKIN  = 100;  // calabaza
constexpr uint16_t SACK1    = 101;  // saco
constexpr uint16_t SACK2    = 102;  // saco dorado
constexpr uint16_t STONE1   = 103;  // piedra
constexpr uint16_t STONE2   = 104;  // piedra 2
constexpr uint16_t STONE3   = 105;  // piedra 3
constexpr uint16_t STONE4   = 106;  // piedra 4
constexpr uint16_t GOLD1    = 107;  // moneda/lingote
constexpr uint16_t GOLD2    = 108;  // moneda/lingote 2
constexpr uint16_t SIGN1    = 109;  // cartel
constexpr uint16_t SIGN2    = 110;  // cartel 2
constexpr uint16_t LAMP     = 111;  // lampara
constexpr uint16_t LOG_S    = 112;  // tronco chico

// Row 7 (tiles 113-128): troncos, leña, flores
constexpr uint16_t LOG1     = 113;  // tronco
constexpr uint16_t LOG2     = 114;  // leña
constexpr uint16_t LOG3     = 115;  // leña apilada
constexpr uint16_t WOOD_P   = 116;  // poste de madera
// Flowers start around 124+
constexpr uint16_t FLOWER_R = 124;  // flor roja
constexpr uint16_t FLOWER_T = 125;  // tulipán
constexpr uint16_t FLOWER_Y = 126;  // flor amarilla

// Row 8 (tiles 129-140): más flores
constexpr uint16_t FLWR_A   = 129;  // flor A
constexpr uint16_t FLWR_B   = 130;  // flor B
constexpr uint16_t FLWR_C   = 131;  // flor C
constexpr uint16_t FLWR_D   = 132;  // flor D
constexpr uint16_t FLWR_E   = 133;  // flor E
constexpr uint16_t FLWR_F   = 134;  // flor F
constexpr uint16_t FLWR_G   = 135;  // flor G
constexpr uint16_t FLWR_H   = 136;  // flor H

// Fences (tiles 141-156) — 4x4 grid del Fences.png
constexpr uint16_t FENCE_V1 = 141;  // vertical post
constexpr uint16_t FENCE_V2 = 145;  // vertical segment
constexpr uint16_t FENCE_H  = 147;  // horizontal fence

// Chest
constexpr uint16_t CHEST    = 157;

// ── Map dimensions ──
constexpr int MW = 40;
constexpr int MH = 30;

// Helper: crea un sprite estático (objeto decorativo grande)
static void makeStaticSprite(eng::ecs::Registry& reg, eng::TextureHandle tex,
                             float x, float y, float w, float h, int layer = 1) {
    auto e = reg.create();
    auto& t = reg.emplace<eng::ecs::Transform2D>(e);
    t.position = {x, y};
    t.prevPosition = t.position;

    auto& s = reg.emplace<eng::ecs::Sprite>(e);
    s.texture = tex;
    s.uvRect  = {0.0f, 0.0f, 1.0f, 1.0f};
    s.width   = w;
    s.height  = h;
    s.layer   = layer;

    // Collider en la base del objeto (ej: tronco del arbol, puerta de la casa).
    // Cubre ~40% del ancho y ~20% del alto, centrado abajo.
    auto& c     = reg.emplace<eng::ecs::BoxCollision>(e);
    c.width     = w * 0.4f;
    c.height    = h * 0.2f;
    c.offsetX   = 0.0f;
    c.offsetY   = h * 0.4f;   // empuja hacia la base
    c.isSolid   = true;
}

// Helper: crea un NPC animado
static void makeAnimatedNPC(eng::ecs::Registry& reg, eng::TextureHandle tex,
                            float x, float y, int cols, int rows, float frameDur) {
    auto e = reg.create();
    auto& t = reg.emplace<eng::ecs::Transform2D>(e);
    t.position = {x, y};
    t.prevPosition = t.position;

    auto& s = reg.emplace<eng::ecs::Sprite>(e);
    s.texture = tex;
    s.uvRect  = {0.0f, 0.0f, 1.0f / (float)cols, 1.0f / (float)rows};
    s.width   = 2.0f;
    s.height  = 2.0f;
    s.layer   = 1;

    // Collider en los pies del NPC, mas chico que el sprite visual.
    auto& c     = reg.emplace<eng::ecs::BoxCollision>(e);
    c.width     = 0.8f;
    c.height    = 0.5f;
    c.offsetX   = 0.0f;
    c.offsetY   = 0.5f;
    c.isSolid   = true;

    auto& a = reg.emplace<eng::ecs::SpriteAnimator>(e);
    a.clips.push_back({"idle", eng::framesFromGrid(cols, rows, 0), frameDur, true});
}

// Helper: dibuja un borde de agua (rectángulo con bordes)
static void makeWaterRect(std::vector<uint16_t>& tiles, int mapW,
                          int x, int y, int w, int h) {
    // Corners
    tiles[y * mapW + x]               = WATER_TL;
    tiles[y * mapW + (x + w - 1)]     = WATER_TR;
    tiles[(y + h - 1) * mapW + x]     = WATER_BL;
    tiles[(y + h - 1) * mapW + (x + w - 1)] = WATER_BR;
    // Edges
    for (int c = x + 1; c < x + w - 1; ++c) {
        tiles[y * mapW + c]           = WATER_T;
        tiles[(y + h - 1) * mapW + c] = WATER_B;
    }
    for (int r = y + 1; r < y + h - 1; ++r) {
        tiles[r * mapW + x]           = WATER_L;
        tiles[r * mapW + (x + w - 1)] = WATER_R;
    }
    // Interior
    for (int r = y + 1; r < y + h - 1; ++r)
        for (int c = x + 1; c < x + w - 1; ++c)
            tiles[r * mapW + c] = WATER_C;
}

// Helper: dibuja una línea de camino horizontal (3 tiles de alto)
static void makePathH(std::vector<uint16_t>& tiles, int mapW,
                      int x1, int x2, int y) {
    tiles[y * mapW + x1]     = PATH_TL;
    tiles[(y+1) * mapW + x1] = PATH_L;
    tiles[(y+2) * mapW + x1] = PATH_BL;
    for (int c = x1 + 1; c < x2; ++c) {
        tiles[y * mapW + c]     = PATH_T;
        tiles[(y+1) * mapW + c] = PATH_C;
        tiles[(y+2) * mapW + c] = PATH_B;
    }
    tiles[y * mapW + x2]     = PATH_TR;
    tiles[(y+1) * mapW + x2] = PATH_R;
    tiles[(y+2) * mapW + x2] = PATH_BR;
}

// Helper: dibuja una línea de camino vertical (3 tiles de ancho)
static void makePathV(std::vector<uint16_t>& tiles, int mapW,
                      int x, int y1, int y2) {
    tiles[y1 * mapW + x]     = PATH_TL;
    tiles[y1 * mapW + x + 1] = PATH_T;
    tiles[y1 * mapW + x + 2] = PATH_TR;
    for (int r = y1 + 1; r < y2; ++r) {
        tiles[r * mapW + x]     = PATH_L;
        tiles[r * mapW + x + 1] = PATH_C;
        tiles[r * mapW + x + 2] = PATH_R;
    }
    tiles[y2 * mapW + x]     = PATH_BL;
    tiles[y2 * mapW + x + 1] = PATH_B;
    tiles[y2 * mapW + x + 2] = PATH_BR;
}

int main(int, char**) {
    eng::Engine engine;
    if (!engine.init()) return 1;

    eng::Input::bind(eng::Action::Pause,     SDL_SCANCODE_P);
    eng::Input::bind(eng::Action::Step,      SDL_SCANCODE_O);
    eng::Input::bind(eng::Action::MoveLeft,  SDL_SCANCODE_A);
    eng::Input::bind(eng::Action::MoveRight, SDL_SCANCODE_D);
    eng::Input::bind(eng::Action::MoveUp,    SDL_SCANCODE_W);
    eng::Input::bind(eng::Action::MoveDown,  SDL_SCANCODE_S);

    auto& reg   = engine.registry();
    auto& sched = engine.scheduler();

    // ================================================================
    // PLAYER
    // ================================================================
    auto player = reg.create();
    reg.emplace<eng::ecs::PlayerTag>(player);
    auto& pt = reg.emplace<eng::ecs::Transform2D>(player);
    auto& pv = reg.emplace<eng::ecs::Velocity2D>(player);

    auto& spr = reg.emplace<eng::ecs::Sprite>(player);
    spr.texture = engine.textures().load("demo/assets/Cute_Fantasy_Free/Player/Player.png");
    spr.uvRect  = {0.0f, 0.0f, 1.0f / 6.0f, 1.0f / 10.0f};
    spr.width   = 2.0f;
    spr.height  = 2.0f;
    spr.tint    = {1.0f, 1.0f, 1.0f, 1.0f};
    spr.layer   = 1;

    // Player comienza en el centro del mapa (near the crossroads)
    pt.position     = {0.0f, 0.0f};
    pt.prevPosition = pt.position;
    pv.velocity     = {0.0f, 0.0f};

    auto& anim = reg.emplace<eng::ecs::SpriteAnimator>(player);
    anim.clips.push_back({"idle",      eng::framesFromGrid(6, 10, 0), 0.2f,  true});
    anim.clips.push_back({"walk_down", eng::framesFromGrid(6, 10, 3), 0.12f, true});
    anim.clips.push_back({"walk_up",   eng::framesFromGrid(6, 10, 5), 0.12f, true});
    anim.clips.push_back({"walk_side", eng::framesFromGrid(6, 10, 4), 0.12f, true});

    // Collider chico en los pies del personaje.
    // offset (0, +0.6) lo baja desde el centro del sprite hacia los pies.
    auto& cl = reg.emplace<eng::ecs::BoxCollision>(player);
    cl.width    = 0.6f;
    cl.height   = 0.4f;
    cl.offsetX  = 0.0f;
    cl.offsetY  = 0.6f;
    cl.isSolid  = true;

    // Camara: entidad separada con limites del mapa.
    // El mapa va de (-MW/2, -MH/2) a (+MW/2, +MH/2) en world coords.
    auto camEnt = reg.create();
    auto& camera = reg.emplace<eng::ecs::Camera>(camEnt);
    camera.position    = pt.position;
    camera.smoothSpeed = 3.0f;         // ajustar al gusto (1=muy lento, 10=rapido)
    camera.mapLeft     = -(float)MW / 2.0f;
    camera.mapRight    =  (float)MW / 2.0f;
    camera.mapTop      = -(float)MH / 2.0f;
    camera.mapBottom   =  (float)MH / 2.0f;

    // ================================================================
    // CARGAR TEXTURAS DE OBJETOS GRANDES
    // ================================================================
    auto texHouse = engine.textures().load("demo/assets/Cute_Fantasy_Free/Outdoor decoration/House_1_Wood_Base_Blue.png");
    auto texTree  = engine.textures().load("demo/assets/Cute_Fantasy_Free/Outdoor decoration/Oak_Tree.png");
    auto texTreeS = engine.textures().load("demo/assets/Cute_Fantasy_Free/Outdoor decoration/Oak_Tree_Small.png");

    auto texSkeleton = engine.textures().load("demo/assets/Cute_Fantasy_Free/Enemies/Skeleton.png");
    auto texChicken  = engine.textures().load("demo/assets/Cute_Fantasy_Free/Animals/Chicken/Chicken.png");
    auto texSheep    = engine.textures().load("demo/assets/Cute_Fantasy_Free/Animals/Sheep/Sheep.png");
    auto texCow      = engine.textures().load("demo/assets/Cute_Fantasy_Free/Animals/Cow/Cow.png");
    auto texPig      = engine.textures().load("demo/assets/Cute_Fantasy_Free/Animals/Pig/Pig.png");

    // ================================================================
    // OBJETOS GRANDES (sprites posicionados en coordenadas de mundo)
    // El mapa se centra en (0,0), va de (-20,-15) a (+20,+15)
    // ================================================================

    // ── Casa del pueblo (esquina noroeste) ──
    // House = 96x128 = 6x8 tiles = 6.0 x 8.0 world units
    makeStaticSprite(reg, texHouse, -14.0f, -10.0f, 6.0f, 8.0f, 1);

    // ── Árboles grandes (bosque al este) ──
    // Oak_Tree = 64x80 = 4x5 tiles = 4.0 x 5.0 world units
    makeStaticSprite(reg, texTree, 12.0f, -12.0f, 4.0f, 5.0f, 1);
    makeStaticSprite(reg, texTree, 15.0f, -11.0f, 4.0f, 5.0f, 1);
    makeStaticSprite(reg, texTree, 13.0f, -8.0f,  4.0f, 5.0f, 1);
    makeStaticSprite(reg, texTree, 16.0f, -7.0f,  4.0f, 5.0f, 1);
    makeStaticSprite(reg, texTree, 11.0f, -6.0f,  4.0f, 5.0f, 1);
    makeStaticSprite(reg, texTree, 14.0f, -4.0f,  4.0f, 5.0f, 1);

    // ── Árboles chicos dispersos ──
    // Oak_Tree_Small = 96x48 pero tiene 2 arbolitos, usamos UV parcial
    // Arbol izquierdo: primeros ~32x48px aprox
    //Sin colision, tengo que ver si son arbustos.
    {
        auto e = reg.create();
        auto& t = reg.emplace<eng::ecs::Transform2D>(e);
        t.position = {-8.0f, -12.0f}; t.prevPosition = t.position;
        auto& s = reg.emplace<eng::ecs::Sprite>(e);
        s.texture = texTreeS;
        s.uvRect = {0.0f, 0.0f, 0.33f, 1.0f}; // primer tercio
        s.width = 2.0f; s.height = 3.0f; s.layer = 1;
    }
    {
        auto e = reg.create();
        auto& t = reg.emplace<eng::ecs::Transform2D>(e);
        t.position = {-5.0f, -11.0f}; t.prevPosition = t.position;
        auto& s = reg.emplace<eng::ecs::Sprite>(e);
        s.texture = texTreeS;
        s.uvRect = {0.33f, 0.0f, 0.33f, 1.0f}; // segundo arbolito (w=0.33, no 0.67)
        s.width = 4.0f; s.height = 3.0f; s.layer = 1;
    }

    // ── Árboles sueltos por el mapa ──
    makeStaticSprite(reg, texTree, -18.0f, 2.0f,  4.0f, 5.0f, 1);
    makeStaticSprite(reg, texTree, 8.0f,   7.0f,  4.0f, 5.0f, 1);

    // ================================================================
    // NPCs Y ANIMALES
    // ================================================================

    // ── Skeleton custodiando el bosque ──
    makeAnimatedNPC(reg, texSkeleton, 10.0f, -5.0f, 6, 10, 0.25f);
    makeAnimatedNPC(reg, texSkeleton, 13.0f, -2.0f, 6, 10, 0.3f);

    // ── Animales en la granja (suroeste) ──
    makeAnimatedNPC(reg, texChicken, -12.0f, 6.0f,  2, 2, 0.3f);
    makeAnimatedNPC(reg, texChicken, -11.0f, 7.0f,  2, 2, 0.35f);
    makeAnimatedNPC(reg, texChicken, -13.0f, 7.5f,  2, 2, 0.28f);
    makeAnimatedNPC(reg, texSheep,   -10.0f, 5.0f,  2, 2, 0.4f);
    makeAnimatedNPC(reg, texSheep,   -9.0f,  6.5f,  2, 2, 0.38f);
    makeAnimatedNPC(reg, texCow,     -14.0f, 4.5f,  2, 2, 0.45f);
    makeAnimatedNPC(reg, texCow,     -13.0f, 5.5f,  2, 2, 0.42f);
    makeAnimatedNPC(reg, texPig,     -11.5f, 4.0f,  2, 2, 0.35f);

    // ── Animales sueltos cerca del lago ──
    makeAnimatedNPC(reg, texChicken, 5.0f,  10.0f, 2, 2, 0.32f);
    makeAnimatedNPC(reg, texSheep,   7.0f,  9.0f,  2, 2, 0.4f);

    // ================================================================
    // TILEMAP (40x30)
    // ================================================================
    auto tilemapEnt = reg.create();
    auto& tmT = reg.emplace<eng::ecs::Transform2D>(tilemapEnt);
    tmT.position     = {-(float)MW / 2.0f, -(float)MH / 2.0f};
    tmT.prevPosition = tmT.position;

    auto& tm = reg.emplace<eng::ecs::Tilemap>(tilemapEnt);
    auto tsTexH = engine.textures().load("demo/assets/tileset.png");
    const auto& tsTex = engine.textures().get(tsTexH);
    tm.tileset = eng::Tileset(tsTexH, 16, 16, tsTex.width, tsTex.height);
    tm.width  = MW;
    tm.height = MH;

    auto setTile = [&](std::vector<uint16_t>& tiles, int col, int row, uint16_t id) {
        if (col >= 0 && col < MW && row >= 0 && row < MH)
            tiles[row * MW + col] = id;
    };

    // ── CAPA 0: Ground (todo pasto) ──
    eng::ecs::TilemapLayer ground;
    ground.renderOrder = 0;
    ground.tiles.resize(MW * MH, GRASS);
    tm.layers.push_back(std::move(ground));

    // ── CAPA 1: Terreno (caminos, agua, cultivo, cliff) ──
    eng::ecs::TilemapLayer terrain;
    terrain.renderOrder = 1;
    terrain.tiles.resize(MW * MH, 0);

    // ── Camino principal horizontal (cruza todo el mapa, fila 14-16) ──
    makePathH(terrain.tiles, MW, 1, 38, 14);

    // ── Camino vertical norte (sube desde el cruce, col 19-21) ──
    makePathV(terrain.tiles, MW, 19, 2, 13);

    // ── Camino vertical sur (baja desde el cruce, col 19-21) ──
    makePathV(terrain.tiles, MW, 19, 17, 27);

    // ── Intersección (rellenar el cruce donde se juntan) ──
    for (int r = 14; r <= 16; ++r)
        for (int c = 19; c <= 21; ++c)
            terrain.tiles[r * MW + c] = PATH_C;

    // ── Lago grande (sureste, 8x6) ──
    makeWaterRect(terrain.tiles, MW, 26, 20, 8, 6);

    // ── Lago chico (noroeste, 4x3) ──
    makeWaterRect(terrain.tiles, MW, 2, 3, 5, 4);

    // ── Zona de cultivo / granja (suroeste, múltiples parcelas de 3x3) ──
    // Parcela 1
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            setTile(terrain.tiles, 3 + c, 20 + r, static_cast<uint16_t>(FARM_TL + r * 3 + c));
    // Parcela 2
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            setTile(terrain.tiles, 7 + c, 20 + r, static_cast<uint16_t>(FARM_TL + r * 3 + c));
    // Parcela 3
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            setTile(terrain.tiles, 3 + c, 24 + r, static_cast<uint16_t>(FARM_TL + r * 3 + c));
    // Parcela 4
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            setTile(terrain.tiles, 7 + c, 24 + r, static_cast<uint16_t>(FARM_TL + r * 3 + c));

    // ── Acantilado (borde norte del bosque, noreste) ──
    // Fila de cliff: TL T T T TR
    setTile(terrain.tiles, 28, 2, CLIFF_TL);
    for (int c = 29; c <= 35; ++c) setTile(terrain.tiles, c, 2, CLIFF_T);
    setTile(terrain.tiles, 36, 2, CLIFF_TR);

    setTile(terrain.tiles, 28, 3, CLIFF_L);
    for (int c = 29; c <= 35; ++c) setTile(terrain.tiles, c, 3, CLIFF_C);
    setTile(terrain.tiles, 36, 3, CLIFF_R);

    setTile(terrain.tiles, 28, 4, CLIFF_BL);
    for (int c = 29; c <= 35; ++c) setTile(terrain.tiles, c, 4, CLIFF_B);
    setTile(terrain.tiles, 36, 4, CLIFF_BR);

    tm.layers.push_back(std::move(terrain));

    // ── CAPA 2: Decoraciones (flores, rocas, arbustos, cajones, etc) ──
    eng::ecs::TilemapLayer decor;
    decor.renderOrder = 2;
    decor.tiles.resize(MW * MH, 0);

    // Flores alrededor de la casa
    setTile(decor.tiles, 5,  5,  FLWR_A);
    setTile(decor.tiles, 6,  5,  FLWR_B);
    setTile(decor.tiles, 7,  5,  FLWR_C);
    setTile(decor.tiles, 5,  6,  FLWR_D);
    setTile(decor.tiles, 7,  6,  FLWR_E);
    setTile(decor.tiles, 8,  5,  FLOWER_R);
    setTile(decor.tiles, 8,  6,  FLOWER_T);

    // Cajones y barriles cerca de la casa
    setTile(decor.tiles, 3,  8,  CRATE1);
    setTile(decor.tiles, 4,  8,  BARREL1);
    setTile(decor.tiles, 3,  9,  CRATE2);
    setTile(decor.tiles, 4,  9,  BARREL2);

    // Rocas dispersas por el mapa
    setTile(decor.tiles, 15, 5,  ROCK_S);
    setTile(decor.tiles, 25, 8,  ROCK_L);
    setTile(decor.tiles, 30, 10, ROCK_S);
    setTile(decor.tiles, 35, 18, ROCK_S);
    setTile(decor.tiles, 18, 25, ROCK_L);

    // Piedras alrededor del lago grande
    setTile(decor.tiles, 25, 21, STONE1);
    setTile(decor.tiles, 34, 20, STONE2);
    setTile(decor.tiles, 35, 25, STONE3);
    setTile(decor.tiles, 25, 25, STONE4);

    // Hongos y calabazas en el bosque
    setTile(decor.tiles, 30, 6,  MUSHROOM);
    setTile(decor.tiles, 33, 7,  MUSHROOM);
    setTile(decor.tiles, 31, 8,  PUMPKIN);

    // Arbustos decorativos
    setTile(decor.tiles, 1,  1,  BUSH_L);
    setTile(decor.tiles, 38, 1,  BUSH_M);
    setTile(decor.tiles, 1,  28, BUSH_S);
    setTile(decor.tiles, 38, 28, BUSH_L);
    setTile(decor.tiles, 10, 10, BUSH_M);
    setTile(decor.tiles, 22, 5,  BUSH_S);

    // Hierbas/weeds
    setTile(decor.tiles, 12, 2,  WEED1);
    setTile(decor.tiles, 16, 8,  WEED2);
    setTile(decor.tiles, 28, 12, WEED3);
    setTile(decor.tiles, 5,  18, WEED1);
    setTile(decor.tiles, 35, 15, WEED2);

    // Tronco cortado
    setTile(decor.tiles, 24, 3,  STUMP);
    setTile(decor.tiles, 11, 18, STUMP);

    // Leña y troncos cerca de la casa
    setTile(decor.tiles, 2,  10, LOG1);
    setTile(decor.tiles, 2,  11, LOG2);

    // Sacos de la granja
    setTile(decor.tiles, 11, 21, SACK1);
    setTile(decor.tiles, 11, 22, SACK2);

    // Plantas/vegetales en la granja
    setTile(decor.tiles, 6,  22, PLANT1);
    setTile(decor.tiles, 10, 22, PLANT2);
    setTile(decor.tiles, 6,  26, PLANT3);
    setTile(decor.tiles, 10, 26, PLANT1);

    // Carteles
    setTile(decor.tiles, 18, 13, SIGN1);   // cartel en el cruce norte
    setTile(decor.tiles, 1,  14, SIGN2);   // cartel al inicio del camino

    // Cofre escondido en el bosque
    setTile(decor.tiles, 32, 5,  CHEST);

    // Lámpara en el camino
    setTile(decor.tiles, 22, 14, LAMP);
    setTile(decor.tiles, 17, 14, LAMP);

    // Flores a lo largo del camino sur
    setTile(decor.tiles, 18, 18, FLWR_A);
    setTile(decor.tiles, 22, 19, FLWR_C);
    setTile(decor.tiles, 18, 22, FLWR_E);
    setTile(decor.tiles, 22, 23, FLWR_G);

    // Flores alrededor del lago chico
    setTile(decor.tiles, 1,  3,  FLOWER_Y);
    setTile(decor.tiles, 7,  4,  FLOWER_R);
    setTile(decor.tiles, 1,  6,  FLOWER_T);

    // Enredaderas
    setTile(decor.tiles, 27, 3,  VINE);
    setTile(decor.tiles, 37, 3,  VINE);

    tm.layers.push_back(std::move(decor));

    // ================================================================
    // TILE COLLISION LAYER
    // ================================================================
    // Marcar tiles solidos: agua y cliff bloquean el paso.
    // Recorremos la capa de terreno (layer 1) y marcamos como solido
    // cualquier tile que sea agua (31-48) o cliff (49-66).
    auto& tcl = reg.emplace<eng::ecs::TileCollisionLayer>(tilemapEnt);
    tcl.solid.resize(MW * MH, false);
    const auto& terrainTiles = tm.layers[1].tiles; // capa de terreno
    for (int i = 0; i < MW * MH; ++i) {
        uint16_t id = terrainTiles[i];
        // Agua: tiles 31-48
        if (id >= 31 && id <= 48) tcl.solid[i] = true;
        // Cliff: tiles 49-66
        if (id >= 49 && id <= 66) tcl.solid[i] = true;
    }

    // ================================================================
    // REGISTRAR SISTEMAS
    // ================================================================
    using Phase = eng::ecs::Phase;
    sched.addSystem("InputSystem",          Phase::Update,          10,     eng::ecs::systems::InputSystem);
    sched.addSystem("PlayerControlSystem",  Phase::FixedUpdate,     150,    eng::ecs::systems::PlayerControlSystem);
    sched.addSystem("MovementSystem",       Phase::FixedUpdate,     200,    eng::ecs::systems::MovementSystem);
    sched.addSystem("CollisionSystem",      Phase::FixedUpdate,     250,    eng::ecs::systems::CollisionSystem);
    sched.addSystem("AnimationSystem",      Phase::Update,          300,    eng::ecs::systems::AnimationSystem);
    sched.addSystem("CameraSystem",         Phase::Render,          50,     eng::ecs::systems::CameraSystem);
    sched.addSystem("RenderSystem",         Phase::Render,          100,    eng::ecs::systems::RenderSystem);
    sched.addSystem("DebugUISystem",        Phase::Update,          900,    eng::ecs::systems::DebugUISystem);
    engine.run();
    engine.shutdown();
    return 0;
}
