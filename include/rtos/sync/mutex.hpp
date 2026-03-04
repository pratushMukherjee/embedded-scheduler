#pragma once

#include "rtos/task.hpp"

#include <list>

namespace rtos {

class Scheduler;

class Mutex {
public:
    Mutex() = default;

    // Returns true if locked, false if already held
    bool try_lock(TaskId caller);

    // Block calling task until mutex available or timeout expires
    // Implements priority inheritance: boosts owner to caller's priority if needed
    bool lock(Scheduler& sched, TaskId caller, TickCount timeout = 0);

    // Release mutex. Restores owner's base priority if it was boosted.
    void unlock(Scheduler& sched);

    bool is_locked() const { return owner_ != kNoTask; }
    TaskId owner() const { return owner_; }
    const std::list<TaskId>& wait_queue() const { return wait_queue_; }

private:
    TaskId owner_{kNoTask};
    Priority original_priority_{kLowestPriority};
    std::list<TaskId> wait_queue_;  // Priority-ordered
};

}  // namespace rtos
