#pragma once
#include "engine/ecs/Registry.h"
#include <glm/vec2.hpp>

namespace eng::ecs::systems {

/// Dibuja todos los Tilemaps visibles usando frustum culling.
/// Llamada desde RenderSystem (no se registra en el scheduler).
///
/// Parametros:
///   reg          - ECS registry
///   alpha        - interpolacion entre frames (0-1)
///   camCenter    - posicion de la camara en world coords
///   screenW/H    - tamano de la ventana en pixeles
void TilemapRenderSystem(Registry& reg, float alpha,
                         glm::vec2 camCenter, int screenW, int screenH);

} // namespace eng::ecs::systems
