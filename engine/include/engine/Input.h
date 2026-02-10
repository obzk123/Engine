#pragma once
#include <SDL.h>
#include <array>
#include <cstdint>
#include "engine/Actions.h"
#include <unordered_map>


namespace eng {

class Input {
public:
    static void init();

    // Se llama por cada SDL_Event (raw)
    static void onEvent(const SDL_Event& e);

    // Se llama 1 vez por frame (Update) para cerrar el frame de input
    static void endFrame();

    // Queries
    static bool isDown(SDL_Scancode sc);
    static bool wasPressed(SDL_Scancode sc);
    static bool wasReleased(SDL_Scancode sc);

    // Mapping
    static void bind(Action action, SDL_Scancode scancode);
    static void unbind(Action action);
    static SDL_Scancode binding(Action action);

    // Queries por acción
    static bool actionDown(Action action);
    static bool actionPressed(Action action);
    static bool actionReleased(Action action);

private:
    static constexpr int KeyCount = SDL_NUM_SCANCODES;

    // Estado “actual” y “previo” para edges
    static inline std::array<uint8_t, KeyCount> s_curr{};
    static inline std::array<uint8_t, KeyCount> s_prev{};

    // Eventos en el frame (opcional; acá no hace falta más)
    static inline bool s_initialized = false;

    static void setKey(SDL_Scancode sc, bool down);

    static inline std::array<SDL_Scancode, static_cast<size_t>(Action::COUNT)> s_actionToKey{};

};

} // namespace eng
