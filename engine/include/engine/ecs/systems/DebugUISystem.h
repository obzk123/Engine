#pragma once
#include "engine/ecs/Registry.h"

namespace eng::ecs::systems {
    /// Sistema de debug que dibuja un panel ImGui con informacion del motor:
    /// FPS, entidades, profiler stats, lista de sistemas, bindings, etc.
    /// Requiere que ImGui::NewFrame() ya haya sido llamado antes de ejecutarse.
    void DebugUISystem(Registry& reg, float dt);
}
