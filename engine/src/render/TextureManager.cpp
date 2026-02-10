#include "engine/render/TextureManager.h"

#include <SDL.h>
#include <cassert>

#ifdef _WIN32
#include <Windows.h>
#endif
#include <glad/glad.h>

// stb_image — la implementacion se compila SOLO en este .cpp.
// Todos los demas archivos solo incluyen el header sin IMPLEMENTATION.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace eng {

void TextureManager::init() {
    // Crear la textura dummy blanca (1x1 pixel, RGBA blanco puro).
    // Vive en handle 0 y se usa cuando:
    //  - Un quad se dibuja sin textura (submitQuad de color solido)
    //  - Una textura falla al cargar (fallback seguro)
    uint32_t whiteId = 0;
    glGenTextures(1, &whiteId);
    glBindTexture(GL_TEXTURE_2D, whiteId);

    const uint32_t white = 0xFFFFFFFF; // RGBA blanco
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, &white);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    Texture dummy;
    dummy.glId   = whiteId;
    dummy.width  = 1;
    dummy.height = 1;
    m_textures.push_back(dummy); // handle 0
}

void TextureManager::shutdown() {
    for (auto& tex : m_textures) {
        if (tex.glId) {
            glDeleteTextures(1, &tex.glId);
            tex.glId = 0;
        }
    }
    m_textures.clear();
    m_cache.clear();
}

TextureHandle TextureManager::load(const std::string& path) {
    // Cache: si ya fue cargada, retornar el handle existente.
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        return it->second;
    }

    // Cargar imagen del disco con stb_image.
    // stbi_load devuelve pixeles en orden top-left, que es lo que OpenGL
    // espera con el default de UV (0,0) = top-left.
    stbi_set_flip_vertically_on_load(false);
    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4); // forzar RGBA
    if (!data) {
        SDL_Log("TextureManager::load failed: %s (%s)", path.c_str(), stbi_failure_reason());
        return 0; // retornar la dummy blanca como fallback
    }

    // Subir a la GPU.
    uint32_t texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Filtros: GL_NEAREST para pixel art crujiente (sin difuminar).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Wrapping: clamp to edge para evitar artefactos en los bordes.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Liberar los pixeles de RAM (ya estan en la GPU).
    stbi_image_free(data);

    // Registrar en el vector y cache.
    Texture tex;
    tex.glId   = texId;
    tex.width  = w;
    tex.height = h;

    TextureHandle handle = static_cast<TextureHandle>(m_textures.size());
    m_textures.push_back(tex);
    m_cache[path] = handle;

    SDL_Log("TextureManager: loaded '%s' (%dx%d) -> handle %u", path.c_str(), w, h, handle);
    return handle;
}

const Texture& TextureManager::get(TextureHandle h) const {
    assert(h < m_textures.size() && "Invalid TextureHandle");
    return m_textures[h];
}

uint32_t TextureManager::glId(TextureHandle h) const {
    assert(h < m_textures.size() && "Invalid TextureHandle");
    return m_textures[h].glId;
}

} // namespace eng
