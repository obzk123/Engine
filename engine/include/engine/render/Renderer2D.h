#pragma once
#include <vector>
#include <cstdint>
#include "engine/ecs/Components.h"

namespace eng {
class Renderer2D {
public:
    Renderer2D() = default;

    void init();
    void shutdown();

    void beginFrame(int screenW, int screenH);
    void setCamera(glm::vec2 centerWorld, float pixelsPerUnit);

    void submitQuad(glm::vec2 centerWorld, float wWorld, float hWorld, eng::ecs::Color4 color);
    void flush();

private:

    struct Vertex {
        float x, y;
        float r, g, b, a;
    };

    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_program = 0;
    int32_t  m_locScreenSize = -1;  // cached uniform location for uScreenSize

    int m_screenW = 1;
    int m_screenH = 1;

    glm::vec2 m_camCenter {0.0f, 0.0f};
    float m_ppu = 64.0f;

    std::vector<Vertex> m_vertices;

    uint32_t compileShader(uint32_t type, const char* src);
    uint32_t linkProgram(uint32_t vs, uint32_t fs);
};

} // namespace eng
