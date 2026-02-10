#pragma once
#include "engine/Profiling.h"
#include "engine/ecs/Registry.h"

#include <functional>
#include <string>
#include <vector>

namespace eng::ecs {

enum class Phase {
    FixedUpdate,
    Update,
    Render
};

struct System {
    std::string name;
    Phase phase;
    int priority = 0;
    bool enabled = true;
    std::function<void(Registry&, float)> fn; // dt para Fixed/Update, alpha para Render si querés
};

class SystemScheduler {
public:
    explicit SystemScheduler(eng::Profiler& profiler)
        : m_profiler(profiler) {}

    void addSystem(std::string name, Phase phase, int priority, std::function<void(Registry&, float)> fn);
    void fixedUpdate(Registry& reg, float fixedDt);
    void update(Registry& reg, float dt);
    void render(Registry& reg, float alpha);
    void sortSystems();
    const std::vector<System>& systems() const { return m_systems; }
    bool setEnabled(const std::string& name, bool enabled);
    bool isEnabled(const std::string& name) const;
private:
    void runPhase(Phase phase, Registry& reg, float dtOrAlpha);

private:
    eng::Profiler& m_profiler;
    std::vector<System> m_systems;
};

} // namespace eng::ecs
