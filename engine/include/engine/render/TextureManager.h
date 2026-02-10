#pragma once
#include "engine/render/Texture.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace eng {

/// Gestiona la carga, cache y liberacion de texturas en la GPU.
///
/// - init() crea la textura dummy blanca en handle 0 (siempre valida).
/// - load() carga una imagen del disco (stb_image), la sube a la GPU y
///   devuelve un TextureHandle. Si el mismo path ya fue cargado, devuelve
///   el handle cacheado.
/// - Filtros: GL_NEAREST por defecto (pixel art / Stardew Valley style).
class TextureManager {
public:
    void init();
    void shutdown();

    /// Carga una textura desde un archivo (PNG, JPG, TGA, BMP).
    /// Retorna handle 0 (dummy blanca) si la carga falla.
    TextureHandle load(const std::string& path);

    /// Retorna los datos de la textura dado un handle.
    const Texture& get(TextureHandle h) const;

    /// Atajo: retorna el GL texture name para bindear.
    uint32_t glId(TextureHandle h) const;

private:
    std::vector<Texture> m_textures;
    std::unordered_map<std::string, TextureHandle> m_cache;
};

} // namespace eng
