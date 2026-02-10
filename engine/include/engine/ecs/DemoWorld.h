#pragma once
#include "engine/ecs/Registry.h"
#include "engine/ecs/SystemScheduler.h"
#include <optional>

namespace eng::ecs {

class DemoWorld {
public:
    void init(SystemScheduler& scheduler);

    Registry& registry() { return m_reg; }
    const Registry& registry() const { return m_reg; }

    std::optional<Entity> player() const { return m_player; }
private:
    Registry m_reg;
    bool m_initialized = false;
    std::optional<Entity> m_player;

};

} // namespace eng::ecs
