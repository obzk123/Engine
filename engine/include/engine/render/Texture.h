#pragma once
#include <cstdint>
#include <vector>

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

/// Genera los Rects UV para una fila de un sprite sheet organizado en grilla.
///   cols, rows  = cuantos frames hay en X e Y en el sheet completo
///   row         = que fila queremos (0 = arriba)
///   startCol    = desde que columna empezar
///   count       = cuantos frames tomar (-1 = todos desde startCol hasta el final)
inline std::vector<Rect> framesFromGrid(int cols, int rows, int row,
                                         int startCol = 0, int count = -1) {
    if (count < 0) count = cols - startCol;
    const float fw = 1.0f / (float)cols;
    const float fh = 1.0f / (float)rows;
    std::vector<Rect> out;
    out.reserve(count);
    for (int i = 0; i < count; ++i) {
        out.push_back({ (startCol + i) * fw, row * fh, fw, fh });
    }
    return out;
}

} // namespace eng
