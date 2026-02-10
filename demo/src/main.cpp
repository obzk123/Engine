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
#include <SDL.h>

// Demo del engine: levanta el motor con 10,000 entidades + player.
// Sirve para testear el engine standalone sin necesidad del proyecto del juego.
//
// Controles:
//   WASD  - Mover al player
//   P     - Pausar/reanudar simulacion
//   O     - Avanzar un fixed step (cuando esta pausado)

int main(int, char**) {
    eng::Engine engine;
    if (!engine.init()) {
        return 1;
    }

    // ── Key bindings (configuracion de la demo, no del motor) ──
    eng::Input::bind(eng::Action::Pause,     SDL_SCANCODE_P);
    eng::Input::bind(eng::Action::Step,      SDL_SCANCODE_O);
    eng::Input::bind(eng::Action::MoveLeft,  SDL_SCANCODE_A);
    eng::Input::bind(eng::Action::MoveRight, SDL_SCANCODE_D);
    eng::Input::bind(eng::Action::MoveUp,    SDL_SCANCODE_W);
    eng::Input::bind(eng::Action::MoveDown,  SDL_SCANCODE_S);

    auto& reg   = engine.registry();
    auto& sched = engine.scheduler();

    // ── Crear 10,000 entidades de prueba ──
    constexpr int N = 10'000;
    for (int i = 0; i < N; ++i) {
        auto e = reg.create();
        auto& t = reg.emplace<eng::ecs::Transform2D>(e);
        auto& v = reg.emplace<eng::ecs::Velocity2D>(e);
        t.position = { float(i % 200), float(i / 200) };
        v.velocity = { 1.0f, 0.0f };
    }

    // ── Crear player (con Sprite texturizado) ──
    auto player = reg.create();
    reg.emplace<eng::ecs::PlayerTag>(player);
    auto& pt = reg.emplace<eng::ecs::Transform2D>(player);
    auto& pv = reg.emplace<eng::ecs::Velocity2D>(player);

    auto& spr = reg.emplace<eng::ecs::Sprite>(player);
    spr.texture = engine.textures().load("demo/assets/player.png");
    spr.uvRect  = {0.0f, 0.0f, 0.25f, 0.25f};  // frame 0 del sheet (1/4 x 1/4)
    spr.width   = 1.0f;
    spr.height  = 1.0f;
    spr.tint    = {1.0f, 1.0f, 1.0f, 1.0f};
    spr.layer   = 1;

    pt.position     = {0.0f, 0.0f};
    pt.prevPosition = {0.0f, 0.0f};
    pv.velocity     = {0.0f, 0.0f};

    // ── Configurar animaciones del player ──
    // El sprite sheet es 4 columnas x 4 filas (cada frame 16x16, total 64x64).
    //   Fila 0: idle       (mirando abajo, sutil)
    //   Fila 1: walk_down  (caminando hacia la camara)
    //   Fila 2: walk_up    (caminando de espaldas)
    //   Fila 3: walk_side  (caminando de costado)
    auto& anim = reg.emplace<eng::ecs::SpriteAnimator>(player);

    // Clip 0: idle
    anim.clips.push_back({"idle",      eng::framesFromGrid(4, 4, 0), 0.3f, true});
    // Clip 1: walk_down
    anim.clips.push_back({"walk_down", eng::framesFromGrid(4, 4, 1), 0.15f, true});
    // Clip 2: walk_up
    anim.clips.push_back({"walk_up",   eng::framesFromGrid(4, 4, 2), 0.15f, true});
    // Clip 3: walk_side (para izquierda y derecha)
    anim.clips.push_back({"walk_side", eng::framesFromGrid(4, 4, 3), 0.15f, true});


    // ── Registrar sistemas ──
    using Phase = eng::ecs::Phase;
    sched.addSystem("InputSystem",         Phase::Update,      10,
                    eng::ecs::systems::InputSystem);
    sched.addSystem("PlayerControlSystem", Phase::FixedUpdate, 150,
                    eng::ecs::systems::PlayerControlSystem);
    sched.addSystem("MovementSystem",      Phase::FixedUpdate, 200,
                    eng::ecs::systems::MovementSystem);
    sched.addSystem("DebugUISystem",       Phase::Update,      900,
                    eng::ecs::systems::DebugUISystem);
    sched.addSystem("RenderSystem",        Phase::Render,      100,
                    eng::ecs::systems::RenderSystem);
    sched.addSystem("AnimationSystem",     Phase::Update,      300, 
                    eng::ecs::systems::AnimationSystem);

    engine.run();
    engine.shutdown();
    return 0;
}
