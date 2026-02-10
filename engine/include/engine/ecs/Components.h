#pragma once
#include <glm/vec2.hpp>

namespace eng::ecs {

struct Transform2D {
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 prevPosition{0.0f, 0.0f};
    float rotation = 0.0f;
};

struct Velocity2D {
    glm::vec2 velocity{0.0f, 0.0f};
};

struct PlayerTag {};

struct Color4 {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct RenderQuad {
    int layer = 0;
    float w = 1.0f;
    float h = 1.0f;
    Color4 color{};
};


} // namespace eng::ecs
