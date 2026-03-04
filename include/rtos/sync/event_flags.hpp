#pragma once

#include "rtos/task.hpp"

#include <list>

namespace rtos {

class Scheduler;

enum class EventWaitMode {
    ANY,  // Wake when ANY of the requested flags are set
    ALL   // Wake when ALL of the requested flags are set
};

class EventFlags {
public:
    EventFlags() = default;

    // Set flags (OR into current flags)
    void set(Scheduler& sched, uint32_t flags);

    // Clear flags
    void clear(uint32_t flags);

    // Non-blocking check
    uint32_t get() const { return flags_; }

    // Block until requested flags are set
    bool wait(Scheduler& sched, TaskId caller, uint32_t requested,
              EventWaitMode mode = EventWaitMode::ANY, TickCount timeout = 0);

    // Non-blocking check
    bool check(uint32_t requested, EventWaitMode mode) const;

private:
    struct Waiter {
        TaskId task_id;
        uint32_t requested_flags;
        EventWaitMode mode;
    };

    uint32_t flags_{0};
    std::list<Waiter> waiters_;
};

}  // namespace rtos
