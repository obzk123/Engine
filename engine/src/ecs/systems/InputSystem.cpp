#include "engine/ecs/systems/InputSystem.h"
#include "engine/Input.h"
#include "engine/Time.h"
#include "engine/Actions.h"

namespace eng::ecs::systems {

void InputSystem(Registry&, float) {
    if (eng::Input::actionPressed(eng::Action::Pause)) {
        eng::Time::setPaused(!eng::Time::isPaused());
    }
    if (eng::Input::actionPressed(eng::Action::Step)) {
        eng::Time::stepOnce();
    }
    // NOTA: Input::endFrame() se llama desde Engine::run() al final del frame,
    // NO desde un sistema. Es infraestructura del motor, no logica de juego.
}

} // namespace eng::ecs::systems
