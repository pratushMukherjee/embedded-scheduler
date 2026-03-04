#pragma once

#include "rtos/task.hpp"

#include <functional>

namespace rtos {

using WatchdogCallback = std::function<void(TaskId)>;

struct Watchdog {
    TaskId task_id{kNoTask};
    TickCount timeout{0};
    TickCount deadline{0};
    WatchdogCallback on_expire;
    bool active{false};
};

}  // namespace rtos
