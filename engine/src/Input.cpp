#include "engine/Input.h"

namespace eng {

void Input::init() {
    if (s_initialized) return;
    s_initialized = true;
    s_actionToKey.fill(SDL_SCANCODE_UNKNOWN);
    s_curr.fill(0);
    s_prev.fill(0);
}

void Input::setKey(SDL_Scancode sc, bool down) {
    if (sc < 0 || sc >= KeyCount) return;
    s_curr[sc] = down ? 1 : 0;
}

void Input::onEvent(const SDL_Event& e) {
    if (!s_initialized) init();

    switch (e.type) {
        case SDL_KEYDOWN:
            if (e.key.repeat == 0) setKey(e.key.keysym.scancode, true);
            break;
        case SDL_KEYUP:
            setKey(e.key.keysym.scancode, false);
            break;
        default:
            break;
    }
}

void Input::endFrame() {
    // Copiamos el estado actual como “previo” para detectar edges en el próximo frame
    s_prev = s_curr;
}

bool Input::isDown(SDL_Scancode sc) {
    if (sc < 0 || sc >= KeyCount) return false;
    return s_curr[sc] != 0;
}

bool Input::wasPressed(SDL_Scancode sc) {
    if (sc < 0 || sc >= KeyCount) return false;
    return (s_curr[sc] != 0) && (s_prev[sc] == 0);
}

bool Input::wasReleased(SDL_Scancode sc) {
    if (sc < 0 || sc >= KeyCount) return false;
    return (s_curr[sc] == 0) && (s_prev[sc] != 0);
}

void Input::bind(Action action, SDL_Scancode scancode) {
    const size_t idx = static_cast<size_t>(action);
    if (idx >= s_actionToKey.size()) return;
    s_actionToKey[idx] = scancode;
}

void Input::unbind(Action action) {
    const size_t idx = static_cast<size_t>(action);
    if (idx >= s_actionToKey.size()) return;
    s_actionToKey[idx] = SDL_SCANCODE_UNKNOWN;
}

SDL_Scancode Input::binding(Action action) {
    const size_t idx = static_cast<size_t>(action);
    if (idx >= s_actionToKey.size()) return SDL_SCANCODE_UNKNOWN;
    return s_actionToKey[idx];
}


static SDL_Scancode actionKey(Action a) {
    return Input::binding(a);
}

bool Input::actionDown(Action action) {
    SDL_Scancode sc = binding(action);
    if (sc == SDL_SCANCODE_UNKNOWN) return false;
    return isDown(sc);
}

bool Input::actionPressed(Action action) {
    SDL_Scancode sc = binding(action);
    if (sc == SDL_SCANCODE_UNKNOWN) return false;
    return wasPressed(sc);
}

bool Input::actionReleased(Action action) {
    SDL_Scancode sc = binding(action);
    if (sc == SDL_SCANCODE_UNKNOWN) return false;
    return wasReleased(sc);
}


} // namespace eng
