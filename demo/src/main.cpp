#include "engine/Engine.h"

// Demo del engine: levanta el motor con el DemoWorld (10,000 entidades + player).
// Sirve para testear el engine standalone sin necesidad del proyecto del juego.
//
// Controles:
//   WASD  - Mover al player
//   P     - Pausar/reanudar simulacion
//   O     - Avanzar un fixed step (cuando esta pausado)

int main(int, char**) {
    eng::Engine engine;
    if (!engine.init()) {
        return 1;
    }

    engine.run();
    engine.shutdown();
    return 0;
}
