#include "engine/render/Renderer2D.h"
#include "engine/ecs/Components.h"
#include <SDL.h>

#include <string>
#include <cassert>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <glad/glad.h>

namespace eng {

Renderer2D& Renderer2D::instance() {
    static Renderer2D r;
    return r;
}

static const char* kVS = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec4 aColor;
out vec4 vColor;

uniform vec2 uScreenSize;     // pixels
void main() {
    // aPos está en pixels ya.
    // Convertimos pixels -> NDC
    vec2 ndc = vec2(
        (aPos.x / uScreenSize.x) * 2.0 - 1.0,
        1.0 - (aPos.y / uScreenSize.y) * 2.0
    );
    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor = aColor;
}
)";

static const char* kFS = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)";

uint32_t Renderer2D::compileShader(uint32_t type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log;
        log.resize((size_t)len);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        SDL_Log("Renderer2D shader compile error: %s", log.c_str());
        glDeleteShader(s);
        return 0;
    }
    return (uint32_t)s;
}

uint32_t Renderer2D::linkProgram(uint32_t vs, uint32_t fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log;
        log.resize((size_t)len);
        glGetProgramInfoLog(p, len, nullptr, log.data());
        SDL_Log("Renderer2D program link error: %s", log.c_str());
        glDeleteProgram(p);
        return 0;
    }
    return (uint32_t)p;
}

void Renderer2D::init() {
    if (m_program != 0) return;

    uint32_t vs = compileShader(GL_VERTEX_SHADER, kVS);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, kFS);
    assert(vs && fs);

    m_program = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    assert(m_program);

    // Cachear uniform locations una sola vez despues de linkear.
    // glGetUniformLocation es lenta (busca por string en el driver).
    // Hacerlo cada frame es desperdicio; la location no cambia.
    m_locScreenSize = glGetUniformLocation(m_program, "uScreenSize");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer2D::shutdown() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_program) glDeleteProgram(m_program);
    m_vbo = m_vao = m_program = 0;
}

void Renderer2D::beginFrame(int screenW, int screenH) {
    m_screenW = (screenW > 0) ? screenW : 1;
    m_screenH = (screenH > 0) ? screenH : 1;
    m_vertices.clear();
}

void Renderer2D::setCamera(glm::vec2 centerWorld, float pixelsPerUnit) {
    m_camCenter = centerWorld;
    m_ppu = (pixelsPerUnit > 1.0f) ? pixelsPerUnit : 1.0f;
}

void Renderer2D::submitQuad(glm::vec2 centerWorld, float wWorld, float hWorld, eng::ecs::Color4 color) {
    // World -> Screen(pixels)
    const float cx = (centerWorld.x - m_camCenter.x) * m_ppu + (float)m_screenW * 0.5f;
    const float cy = (centerWorld.y - m_camCenter.y) * m_ppu + (float)m_screenH * 0.5f;

    const float hw = (wWorld * m_ppu) * 0.5f;
    const float hh = (hWorld * m_ppu) * 0.5f;

    const float x0 = cx - hw;
    const float y0 = cy - hh;
    const float x1 = cx + hw;
    const float y1 = cy + hh;

    // 2 triángulos (6 vertices)
    Vertex v[6] = {
        { x0, y0, color.r, color.g, color.b, color.a },
        { x1, y0, color.r, color.g, color.b, color.a },
        { x1, y1, color.r, color.g, color.b, color.a },

        { x0, y0, color.r, color.g, color.b, color.a },
        { x1, y1, color.r, color.g, color.b, color.a },
        { x0, y1, color.r, color.g, color.b, color.a },
    };

    m_vertices.insert(m_vertices.end(), std::begin(v), std::end(v));
}

void Renderer2D::flush() {
    if (m_vertices.empty()) return;

    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(m_vertices.size() * sizeof(Vertex)),
                 m_vertices.data(),
                 GL_DYNAMIC_DRAW);

    glUniform2f(m_locScreenSize, (float)m_screenW, (float)m_screenH);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace eng
