#pragma once
#include <glm/vec2.hpp>

namespace eng {

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

inline glm::vec2 lerpVec2(const glm::vec2& a, const glm::vec2& b, float t) {
    return { lerp(a.x, b.x, t), lerp(a.y, b.y, t) };
}

} // namespace eng
