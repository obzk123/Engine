#pragma once
#include <cstdint>

namespace eng::ecs {

struct Entity {
    uint32_t index = 0;
    uint32_t generation = 0;

    static constexpr uint32_t InvalidIndex = 0xFFFFFFFFu;

    static Entity invalid() { return Entity{InvalidIndex, 0}; }
    bool isValid() const { return index != InvalidIndex; }

    friend bool operator==(const Entity& a, const Entity& b) {
        return a.index == b.index && a.generation == b.generation;
    }
    friend bool operator!=(const Entity& a, const Entity& b) {
        return !(a == b);
    }
};

} // namespace eng::ecs
