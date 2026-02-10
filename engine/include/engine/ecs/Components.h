#pragma once
#include <glm/vec2.hpp>
#include <string>
#include <vector>
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

/// Clip de animacion: una secuencia de frames UV dentro de un sprite sheet.
/// Cada frame es un Rect con las coordenadas UV de ese frame en la textura.
struct AnimationClip {
    std::string name;                 // "idle", "walk_down", etc.
    std::vector<eng::Rect> frames;    // UVs de cada frame
    float frameDuration = 0.15f;      // segundos por frame (~6 FPS)
    bool loop = true;                 // repetir al terminar?
};

struct SpriteAnimator {
    std::vector<AnimationClip> clips;
    int currentClip = 0;
    int currentFrame = 0;
    float timer = 0.0f;
    bool playing = true;
};

} // namespace eng::ecs
