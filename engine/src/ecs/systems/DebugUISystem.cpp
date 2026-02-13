#include "engine/ecs/systems/DebugUISystem.h"
#include "engine/ecs/Components.h"
#include "engine/ecs/SystemScheduler.h"
#include "engine/Profiling.h"
#include "engine/Input.h"
#include "engine/Time.h"

#include <imgui.h>
#include <SDL.h>

namespace eng::ecs::systems {

void DebugUISystem(Registry& reg, float /*dt*/) {
    auto& ctx = reg.ctx();

    ImGui::Begin("Debug");

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("Entities alive: %zu", reg.aliveCount());
    ImGui::Text("Transform2D: %zu", reg.componentCount<Transform2D>());
    ImGui::Text("Velocity2D: %zu", reg.componentCount<Velocity2D>());
    ImGui::Text("Fixed dt: %.4f", eng::Time::fixedDeltaTime());

    auto e0 = Entity{0, 0};
    if (reg.isAlive(e0) && reg.has<Transform2D>(e0)) {
        auto& t = reg.get<Transform2D>(e0);
        ImGui::Text("E0 pos: (%.2f, %.2f)", t.position.x, t.position.y);
    }

    ImGui::Text("Simulation: %s", eng::Time::isPaused() ? "PAUSED" : "RUNNING");

    // Profiler
    if (ctx.profiler) {
        auto* profiler = ctx.profiler;
        static int window = (int)profiler->window();
        ImGui::SliderInt("Profiler window", &window, 10, 600);
        profiler->setWindow((size_t)window);

        ImGui::Separator();
        ImGui::Text("Profiler (rolling window: %zu)", profiler->window());

        for (const auto& kv : profiler->stats()) {
            const auto& name = kv.first;
            const auto& st = kv.second;

            ImGui::BulletText(
                "%s | last: %.3f ms | avg: %.3f ms | min: %.3f | max: %.3f | n=%zu",
                name.c_str(), st.lastMs, st.avgMs, st.minMs, st.maxMs, st.samples
            );
        }
    }

    // Scheduler / systems list
    if (ctx.scheduler) {
        auto* scheduler = ctx.scheduler;
        ImGui::Separator();
        ImGui::Text("Systems order:");
        for (const auto& s : scheduler->systems()) {
            bool enabled = s.enabled;

            ImGui::PushID(s.name.c_str());
            if (ImGui::Checkbox("##enabled", &enabled)) {
                scheduler->setEnabled(s.name, enabled);
            }
            ImGui::SameLine();
            ImGui::Text("[%s] p=%d  %s",
                (s.phase == Phase::FixedUpdate) ? "Fixed" :
                (s.phase == Phase::Update)      ? "Update" : "Render",
                s.priority,
                s.name.c_str()
            );
            ImGui::PopID();
        }
    }

    // Key bindings
    ImGui::Separator();
    ImGui::Text("Bindings:");
    SDL_Scancode sc = eng::Input::binding(eng::Action::Pause);
    const char* name = SDL_GetScancodeName(sc);
    ImGui::Text("Pause -> %s", name && name[0] ? name : "<unbound>");

    SDL_Scancode step = eng::Input::binding(eng::Action::Step);
    const char* nameStep = SDL_GetScancodeName(step);
    ImGui::Text("Step  -> %s", nameStep && nameStep[0] ? nameStep : "<unbound>");

    // Player position
    ImGui::Separator();
    auto view = reg.view<PlayerTag, Transform2D>();
    if (view.valid()) {
        auto [e, tag, t] = *view.begin();
        (void)e; (void)tag;
        ImGui::Text("Player pos: (%.2f, %.2f)", t.position.x, t.position.y);
    }

    // Interpolation / accumulator
    ImGui::Separator();
    ImGui::Text("alpha: %.3f", eng::Time::interpolation());
    ImGui::Text("accum: %.4f", eng::Time::getAccumulator());

    ImGui::End();
}

} // namespace eng::ecs::systems
