#pragma once
#include <glm/vec2.hpp>
#include "engine/render/Texture.h"

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

/// Quad de color solido (sin textura). Sigue funcionando como antes.
struct RenderQuad {
    int layer = 0;
    float w = 1.0f;
    float h = 1.0f;
    Color4 color{};
};

/// Sprite texturizado. Usa un TextureHandle del TextureManager.
/// Si texture == 0, dibuja la textura dummy blanca (equivale a un quad blanco).
/// uvRect selecciona una region de la textura (para sprite sheets/atlas).
struct Sprite {
    eng::TextureHandle texture  = 0;      // handle 0 = textura dummy blanca
    eng::Rect          uvRect   = {};     // {0,0,1,1} = toda la textura
    Color4             tint     = {};     // blanco = sin tinteo
    int                layer    = 0;      // orden de dibujo
    float              width    = 1.0f;   // tamano en world units
    float              height   = 1.0f;   // tamano en world units
};

} // namespace eng::ecs
