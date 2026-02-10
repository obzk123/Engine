#include "engine/ecs/DemoWorld.h"
#include "engine/ecs/Components.h"
#include "engine/ecs/systems/InputSystem.h"
#include "engine/ecs/systems/PlayerControlSystem.h"
#include "engine/ecs/systems/RenderSystem.h"

namespace eng::ecs {

static void MovementSystem(Registry& reg, float dt) {
    auto view = reg.view<Transform2D, Velocity2D>();
    for (auto [e, t, v] : view) {
        t.prevPosition = t.position;
        t.position.x += v.velocity.x * dt;
        t.position.y += v.velocity.y * dt;
    }
}

void DemoWorld::init(SystemScheduler& scheduler) {
    if (m_initialized) return;
    m_initialized = true;

    constexpr int N = 10'000;

    for (int i = 0; i < N; ++i) {
        Entity e = m_reg.create();
        auto& t = m_reg.emplace<Transform2D>(e);
        auto& v = m_reg.emplace<Velocity2D>(e);

        t.position = { float(i % 200), float(i / 200) };
        v.velocity = { 1.0f, 0.0f };
    }

    Entity p = m_reg.create();
    m_reg.emplace<PlayerTag>(p);

    auto& t = m_reg.emplace<Transform2D>(p);
    auto& v = m_reg.emplace<Velocity2D>(p);
    auto& rq = m_reg.emplace<RenderQuad>(p);
    rq.w = 1.0f;
    rq.h = 1.0f;
    rq.color = { 0.2f, 1.0f, 0.2f, 1.0f };
    rq.layer = 1;
    t.position = { 0.0f, 0.0f };
    t.prevPosition = {0.0f, 0.0f};
    v.velocity = { 0.0f, 0.0f };

    m_player = p;
    // Registrar sistemas
    scheduler.addSystem("InputSystem",     Phase::Update,      10,  eng::ecs::systems::InputSystem);
    scheduler.addSystem("PlayerControlSystem", Phase::FixedUpdate, 150, eng::ecs::systems::PlayerControlSystem);
    scheduler.addSystem("MovementSystem",  Phase::FixedUpdate, 200, MovementSystem);
    //scheduler.addSystem("PhysicsSystem",   Phase::FixedUpdate, 300, PhysicsSystem);
    //scheduler.addSystem("GameplaySystem",  Phase::FixedUpdate, 400, GameplaySystem);
    //scheduler.addSystem("NetClientSystem", Phase::FixedUpdate, 500, NetClientSystem);
    scheduler.addSystem("RenderSystem",    Phase::Render,      100, eng::ecs::systems::RenderSystem);
}

} // namespace eng::ecs
