#pragma once
#include <cstdint>

namespace eng {

enum class Action : uint16_t {
    Pause,
    Step,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,

    COUNT
};

} // namespace eng
