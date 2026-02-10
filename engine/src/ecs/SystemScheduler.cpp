#include "engine/ecs/SystemScheduler.h"
#include <algorithm>

namespace eng::ecs {

void SystemScheduler::runPhase(Phase phase, Registry& reg, float dtOrAlpha) {
    for (auto& s : m_systems) {
        if (s.phase != phase) continue;
        if (!s.enabled) continue;

        eng::ScopeTimer t(m_profiler, s.name);
        s.fn(reg, dtOrAlpha);
    }
}

void SystemScheduler::fixedUpdate(Registry& reg, float fixedDt) {
    runPhase(Phase::FixedUpdate, reg, fixedDt);
}

void SystemScheduler::update(Registry& reg, float dt) {
    runPhase(Phase::Update, reg, dt);
}

void SystemScheduler::render(Registry& reg, float alpha) {
    runPhase(Phase::Render, reg, alpha);
}

void SystemScheduler::addSystem(std::string name, Phase phase, int priority,
                                std::function<void(Registry&, float)> fn) {
    m_systems.push_back(System{ std::move(name), phase, priority, true, std::move(fn) });
    sortSystems();
}

void SystemScheduler::sortSystems() {
    std::stable_sort(m_systems.begin(), m_systems.end(),
        [](const System& a, const System& b) {
            if (a.phase != b.phase) return a.phase < b.phase;     // agrupa por fase
            if (a.priority != b.priority) return a.priority < b.priority; // prioridad
            return a.name < b.name; // desempate estable
        });
}

bool SystemScheduler::setEnabled(const std::string& name, bool enabled) {
    for (auto& s : m_systems) {
        if (s.name == name) {
            s.enabled = enabled;
            return true;
        }
    }
    return false;
}

bool SystemScheduler::isEnabled(const std::string& name) const {
    for (const auto& s : m_systems) {
        if (s.name == name) return s.enabled;
    }
    return false;
}

} // namespace eng::ecs
