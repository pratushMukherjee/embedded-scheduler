#pragma once

#include "rtos/task.hpp"

#include <functional>

namespace rtos {

using TimerCallback = std::function<void()>;

enum class TimerMode {
    ONE_SHOT,
    PERIODIC
};

struct Timer {
    uint32_t id{0};
    std::string name;
    TimerMode mode{TimerMode::ONE_SHOT};
    TickCount period{0};
    TickCount next_fire{0};
    TimerCallback callback;
    bool active{false};
};

}  // namespace rtos
