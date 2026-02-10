#include "engine/render/Renderer2D.h"
#include "engine/ecs/Components.h"
#include <SDL.h>

#include <string>
#include <cassert>
#include <cmath>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <glad/glad.h>

namespace eng {

// ────────────────────────────────────────────────────────────────
// Shaders
// ────────────────────────────────────────────────────────────────

static const char* kVS = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec4 aColor;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in float aTexIndex;

out vec4 vColor;
out vec2 vTexCoord;
flat out float vTexIndex;

uniform vec2 uScreenSize; // pixels

void main() {
    // aPos esta en pixels ya. Convertimos pixels -> NDC
    vec2 ndc = vec2(
        (aPos.x / uScreenSize.x) * 2.0 - 1.0,
        1.0 - (aPos.y / uScreenSize.y) * 2.0
    );
    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor    = aColor;
    vTexCoord = aTexCoord;
    vTexIndex = aTexIndex;
}
)";

static const char* kFS = R"(
#version 330 core
in vec4 vColor;
in vec2 vTexCoord;
flat in float vTexIndex;

out vec4 FragColor;

uniform sampler2D uTextures[16];

void main() {
    // Samplear la textura del slot indicado y multiplicar por el color.
    // Si es la textura dummy blanca (1x1), el resultado es solo el color.
    // Si el color es blanco, el resultado es solo la textura.
    int idx = int(round(vTexIndex));
    vec4 texColor = texture(uTextures[idx], vTexCoord);
    FragColor = texColor * vColor;
}
)";

// ────────────────────────────────────────────────────────────────
// Shader compile/link (sin cambios)
// ────────────────────────────────────────────────────────────────

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

// ────────────────────────────────────────────────────────────────
// Init / Shutdown
// ────────────────────────────────────────────────────────────────

void Renderer2D::init() {
    if (m_program != 0) return;

    uint32_t vs = compileShader(GL_VERTEX_SHADER, kVS);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, kFS);
    assert(vs && fs);

    m_program = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    assert(m_program);

    // Cachear uniform locations
    m_locScreenSize = glGetUniformLocation(m_program, "uScreenSize");

    // Setear los samplers uTextures[i] = i (una sola vez, despues de linkear)
    glUseProgram(m_program);
    for (int i = 0; i < MaxTextureSlots; ++i) {
        std::string name = "uTextures[" + std::to_string(i) + "]";
        GLint loc = glGetUniformLocation(m_program, name.c_str());
        glUniform1i(loc, i);
    }
    glUseProgram(0);

    // ── VAO / VBO ──
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // location 0: position (x, y)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, x));

    // location 1: color (r, g, b, a)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, r));

    // location 2: texCoord (u, v)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, u));

    // location 3: texIndex
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, texIndex));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ── Textura dummy blanca (1x1) para quads de color solido ──
    glGenTextures(1, &m_whiteTexture);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    const uint32_t white = 0xFFFFFFFF;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ── Alpha blending (necesario para texturas con transparencia) ──
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer2D::shutdown() {
    if (m_whiteTexture) glDeleteTextures(1, &m_whiteTexture);
    if (m_vbo)          glDeleteBuffers(1, &m_vbo);
    if (m_vao)          glDeleteVertexArrays(1, &m_vao);
    if (m_program)      glDeleteProgram(m_program);
    m_whiteTexture = 0;
    m_vbo = m_vao = m_program = 0;
}

// ────────────────────────────────────────────────────────────────
// Frame management
// ────────────────────────────────────────────────────────────────

void Renderer2D::beginFrame(int screenW, int screenH) {
    m_screenW = (screenW > 0) ? screenW : 1;
    m_screenH = (screenH > 0) ? screenH : 1;
    m_vertices.clear();

    // Resetear texture slots — slot 0 siempre es la textura dummy blanca
    m_textureSlots.fill(0);
    m_textureSlots[0] = m_whiteTexture;
    m_textureSlotCount = 1;
}

void Renderer2D::setCamera(glm::vec2 centerWorld, float pixelsPerUnit) {
    m_camCenter = centerWorld;
    m_ppu = (pixelsPerUnit > 1.0f) ? pixelsPerUnit : 1.0f;
}

// ────────────────────────────────────────────────────────────────
// Texture slot management
// ────────────────────────────────────────────────────────────────

float Renderer2D::getTextureSlot(uint32_t glTexId) {
    // Buscar si ya esta en los slots activos
    for (int i = 0; i < m_textureSlotCount; ++i) {
        if (m_textureSlots[i] == glTexId) {
            return (float)i;
        }
    }

    // Si los slots estan llenos, hacer un flush intermedio
    if (m_textureSlotCount >= MaxTextureSlots) {
        flushBatch();
        // Despues del flush, resetear slots (slot 0 = white)
        m_textureSlots.fill(0);
        m_textureSlots[0] = m_whiteTexture;
        m_textureSlotCount = 1;

        // Si la textura que busco es la white, retornar 0
        if (glTexId == m_whiteTexture) return 0.0f;
    }

    // Asignar nuevo slot
    int slot = m_textureSlotCount;
    m_textureSlots[slot] = glTexId;
    m_textureSlotCount++;
    return (float)slot;
}

// ────────────────────────────────────────────────────────────────
// Submit quads
// ────────────────────────────────────────────────────────────────

void Renderer2D::submitQuad(glm::vec2 centerWorld, float wWorld, float hWorld,
                             eng::ecs::Color4 color) {
    // Un quad de color solido es un textured quad con la textura dummy blanca
    // y UV que cubren toda la textura (0,0)-(1,1).
    submitTexturedQuad(centerWorld, wWorld, hWorld, m_whiteTexture, {0, 0, 1, 1}, color);
}

void Renderer2D::submitTexturedQuad(glm::vec2 centerWorld, float wWorld, float hWorld,
                                     uint32_t glTexId, const Rect& uv,
                                     eng::ecs::Color4 tint) {
    float texIdx = getTextureSlot(glTexId);

    // World -> Screen(pixels)
    const float cx = (centerWorld.x - m_camCenter.x) * m_ppu + (float)m_screenW * 0.5f;
    const float cy = (centerWorld.y - m_camCenter.y) * m_ppu + (float)m_screenH * 0.5f;

    const float hw = (wWorld * m_ppu) * 0.5f;
    const float hh = (hWorld * m_ppu) * 0.5f;

    // Pixel-snap: redondear las ESQUINAS a pixel entero.
    // Esto garantiza que cada texel mapea exactamente a un numero entero de
    // pixeles de pantalla, evitando artefactos con GL_NEAREST.
    const float x0 = std::floor(cx - hw);
    const float y0 = std::floor(cy - hh);
    const float x1 = std::floor(cx + hw);
    const float y1 = std::floor(cy + hh);

    // UV corners
    const float u0 = uv.x;
    const float v0 = uv.y;
    const float u1 = uv.x + uv.w;
    const float v1 = uv.y + uv.h;

    // 2 triangulos (6 vertices)
    Vertex v[6] = {
        { x0, y0, tint.r, tint.g, tint.b, tint.a, u0, v0, texIdx },
        { x1, y0, tint.r, tint.g, tint.b, tint.a, u1, v0, texIdx },
        { x1, y1, tint.r, tint.g, tint.b, tint.a, u1, v1, texIdx },

        { x0, y0, tint.r, tint.g, tint.b, tint.a, u0, v0, texIdx },
        { x1, y1, tint.r, tint.g, tint.b, tint.a, u1, v1, texIdx },
        { x0, y1, tint.r, tint.g, tint.b, tint.a, u0, v1, texIdx },
    };

    m_vertices.insert(m_vertices.end(), std::begin(v), std::end(v));
}

// ────────────────────────────────────────────────────────────────
// Flush
// ────────────────────────────────────────────────────────────────

void Renderer2D::flushBatch() {
    if (m_vertices.empty()) return;

    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(m_vertices.size() * sizeof(Vertex)),
                 m_vertices.data(),
                 GL_DYNAMIC_DRAW);

    glUniform2f(m_locScreenSize, (float)m_screenW, (float)m_screenH);

    // Bindear todas las texturas activas a sus texture units
    for (int i = 0; i < m_textureSlotCount; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureSlots[i]);
    }

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertices.size());

    // Limpiar bindings
    for (int i = 0; i < m_textureSlotCount; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    m_vertices.clear();
}

void Renderer2D::flush() {
    flushBatch();
}

} // namespace eng
