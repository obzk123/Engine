#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include "engine/ecs/Components.h"
#include "engine/render/Texture.h"

namespace eng {

class Renderer2D {
public:
    Renderer2D() = default;

    void init();
    void shutdown();

    void beginFrame(int screenW, int screenH);
    void setCamera(glm::vec2 centerWorld, float pixelsPerUnit);

    /// Quad de color solido (compatible con el renderer anterior).
    /// Internamente usa la textura dummy blanca.
    void submitQuad(glm::vec2 centerWorld, float wWorld, float hWorld,
                    eng::ecs::Color4 color);

    /// Quad texturizado con UV rect y color de tinteo.
    /// glTexId es el OpenGL texture name (obtenido de TextureManager::glId).
    void submitTexturedQuad(glm::vec2 centerWorld, float wWorld, float hWorld,
                            uint32_t glTexId, const Rect& uv,
                            eng::ecs::Color4 tint = {1, 1, 1, 1});

    void flush();

private:
    struct Vertex {
        float x, y;
        float r, g, b, a;
        float u, v;
        float texIndex;
    };

    // ── Texture slot management ──
    // Hasta 16 texturas distintas pueden estar bindeadas en un batch.
    // Si se superan, se hace un flush intermedio.
    static constexpr int MaxTextureSlots = 16;
    std::array<uint32_t, MaxTextureSlots> m_textureSlots{};
    int m_textureSlotCount = 0;

    uint32_t m_whiteTexture = 0; // GL ID de la textura 1x1 blanca del renderer

    // ── GL state ──
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_program = 0;
    int32_t  m_locScreenSize = -1;

    int m_screenW = 1;
    int m_screenH = 1;

    glm::vec2 m_camCenter{0.0f, 0.0f};
    float m_ppu = 64.0f;

    std::vector<Vertex> m_vertices;

    uint32_t compileShader(uint32_t type, const char* src);
    uint32_t linkProgram(uint32_t vs, uint32_t fs);

    /// Encuentra o asigna un slot de textura para el GL ID dado.
    /// Si los slots estan llenos, hace un flush intermedio y resetea.
    float getTextureSlot(uint32_t glTexId);

    /// Flush interno (no resetea camera/screen).
    void flushBatch();
};

} // namespace eng
