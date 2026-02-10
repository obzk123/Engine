#include "engine/Time.h"
#include <SDL.h>
#include <algorithm>

namespace eng {

    static uint64_t g_lastCounter = 0;
    static double g_frequency = 0.0;

    void Time::init() {
        g_frequency = static_cast<double>(SDL_GetPerformanceFrequency());
        g_lastCounter = SDL_GetPerformanceCounter();
    }

    void Time::beginFrame() {
        uint64_t counter = SDL_GetPerformanceCounter();
        uint64_t diff = counter - g_lastCounter;
        g_lastCounter = counter;

        s_deltaTime = static_cast<float>(diff / g_frequency);
        s_deltaTime = std::min(s_deltaTime, 0.25f); // evita espiral de la muerte

        if (!s_paused) {
            s_accumulator += s_deltaTime;
        }

        s_frame++;
    }

    float Time::deltaTime() {
        return s_deltaTime;
    }

    float Time::fixedDeltaTime() {
        return s_fixedDelta;
    }

    bool Time::consumeFixedStep() {
        if (!s_paused) {
            if (s_accumulator >= s_fixedDelta) {
                s_accumulator -= s_fixedDelta;
                return true;
            }
            return false;
        }

        if (s_stepRequested) {
            s_stepRequested = false;
            return true;
        }
        return false;
    }


float Time::interpolation() {
    if (s_paused) return 1.0f;
    if (s_fixedDelta <= 0.0f) return 0.0f;
    float a = s_accumulator / s_fixedDelta;
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
}


    uint64_t Time::frameCount() {
        return s_frame;
    }

    void Time::setPaused(bool paused) {
        s_paused = paused;
    }

    bool Time::isPaused() {
        return s_paused;
    }

    void Time::stepOnce() {
        s_stepRequested = true;
    }

}
