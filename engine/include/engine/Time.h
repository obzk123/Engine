#pragma once
#include <stdint.h>
namespace eng {

class Time {
public:
    static void init();
    static void beginFrame();

    static float deltaTime();
    static float fixedDeltaTime();
    static uint64_t frameCount();

    static bool consumeFixedStep();
    static float interpolation();

    static void setPaused(bool paused);
    static bool isPaused();

    static void stepOnce();

    static const float getAccumulator() { return s_accumulator; }

private:
    static inline float s_deltaTime = 0.0f;
    static inline float s_fixedDelta = 1.0f / 60.0f;
    static inline float s_accumulator = 0.0f;
    static inline uint64_t s_frame = 0;

    static inline bool s_paused = false;
    static inline bool s_stepRequested = false;
};

}
