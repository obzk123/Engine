#pragma once
#include <cstdint>

namespace eng {

/// Handle opaco a una textura. Es un indice en el TextureManager.
/// 0 = textura dummy blanca (siempre valida).
using TextureHandle = uint32_t;
static constexpr TextureHandle InvalidTexture = UINT32_MAX;

/// Rectangulo en coordenadas UV (0-1) para seleccionar una region de una textura.
/// Usado para sprite sheets: cada sprite es un Rect dentro del atlas.
struct Rect {
    float x = 0.0f;   // U minimo (izquierda)
    float y = 0.0f;   // V minimo (arriba)
    float w = 1.0f;   // ancho en UV
    float h = 1.0f;   // alto en UV
};

/// Datos de una textura subida a la GPU.
struct Texture {
    uint32_t glId   = 0;    // OpenGL texture name (glGenTextures)
    int      width  = 0;    // Ancho en pixeles
    int      height = 0;    // Alto en pixeles
};

} // namespace eng
