#include "engine/ecs/Registry.h"

namespace eng::ecs {

Entity Registry::create() {
    uint32_t index;

    if (!m_freeList.empty()) {
        index = m_freeList.back();
        m_freeList.pop_back();
    } else {
        index = static_cast<uint32_t>(m_slots.size());
        m_slots.push_back(Slot{});
    }

    Slot& slot = m_slots[index];
    slot.alive = true;

    m_aliveCount++;
    return Entity{ index, slot.generation };
}

bool Registry::isAlive(Entity e) const {
    if (!e.isValid()) return false;
    if (e.index >= m_slots.size()) return false;

    const Slot& slot = m_slots[e.index];
    return slot.alive && slot.generation == e.generation;
}

void Registry::removeAllComponents(Entity e) {
    for (auto& kv : m_pools) {
        kv.second->removeIfExists(e);
    }
}

void Registry::destroy(Entity e) {
    if (!isAlive(e)) return;

    removeAllComponents(e);

    Slot& slot = m_slots[e.index];
    slot.alive = false;
    slot.generation++;          // invalida handles viejos
    m_freeList.push_back(e.index);

    m_aliveCount--;
}

void Registry::clear() {
    for (auto& kv : m_pools) {
        kv.second->clear();
    }
    m_pools.clear();
    m_slots.clear();
    m_freeList.clear();
    m_aliveCount = 0;
}

} // namespace eng::ecs
