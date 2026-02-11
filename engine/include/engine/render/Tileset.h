#pragma once
#include "engine/render/Texture.h"

namespace eng {

/// Tileset: sprite sheet de tiles organizados en grilla uniforme.
/// Cada tile tiene el mismo tamano (ej: 16x16 pixeles).
///
/// Convencion de indices:
///   0      = tile vacio (no dibujar)
///   1..N   = tiles reales, row-major (izquierda a derecha, arriba a abajo)
///
/// Ejemplo: tileset de 256x256 con tiles de 16x16 = 16 cols x 16 rows = 256 tiles.
///   Tile 1 = (col 0, row 0), Tile 2 = (col 1, row 0), ...
class Tileset {
public:
    Tileset() = default;

    /// texture  - Handle del TextureManager
    /// tileW/H  - Tamano de cada tile en pixeles
    /// texW/H   - Tamano de la textura completa en pixeles
    Tileset(TextureHandle texture, int tileW, int tileH, int texW, int texH)
        : m_texture(texture)
        , m_tileW(tileW), m_tileH(tileH)
        , m_texW(texW), m_texH(texH)
        , m_cols(texW / tileW), m_rows(texH / tileH) {}

    /// Retorna el Rect UV del tile con half-pixel inset para evitar bleeding.
    /// Index 0 retorna Rect vacio (w=0, h=0).
    Rect getTileUV(uint16_t tileIndex) const {
        if (tileIndex == 0) return {0.0f, 0.0f, 0.0f, 0.0f};

        uint16_t idx = tileIndex - 1;
        int col = idx % m_cols;
        int row = idx / m_cols;
        float uvW = 1.0f / static_cast<float>(m_cols);
        float uvH = 1.0f / static_cast<float>(m_rows);

        return { col * uvW, row * uvH, uvW, uvH };
    }

    TextureHandle texture() const { return m_texture; }
    int cols() const { return m_cols; }
    int rows() const { return m_rows; }
    int tileCount() const { return m_cols * m_rows; }

private:
    TextureHandle m_texture = 0;
    int m_tileW = 16, m_tileH = 16;
    int m_texW = 0, m_texH = 0;
    int m_cols = 1, m_rows = 1;
};

} // namespace eng
