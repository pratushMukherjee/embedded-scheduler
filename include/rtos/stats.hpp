#pragma once

#include "rtos/task.hpp"

#include <map>

namespace rtos {

struct TaskStats {
    TickCount ticks_run{0};
    uint64_t context_switches{0};
    double cpu_percent{0.0};
    std::size_t stack_high_water{0};
};

struct SystemStats {
    TickCount total_ticks{0};
    uint64_t total_context_switches{0};
    TickCount idle_ticks{0};
    double cpu_utilization{0.0};
    std::map<TaskId, TaskStats> per_task;
};

}  // namespace rtos
