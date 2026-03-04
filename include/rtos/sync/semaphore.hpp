#pragma once

#include "rtos/task.hpp"

#include <list>

namespace rtos {

class Scheduler;

class Semaphore {
public:
    explicit Semaphore(uint32_t initial_count, uint32_t max_count = UINT32_MAX);

    // Returns true if acquired, false if would block
    bool try_take();

    // Block calling task until semaphore available or timeout expires
    // Returns true if acquired, false on timeout
    bool take(Scheduler& sched, TaskId caller, TickCount timeout = 0);

    void give(Scheduler& sched);

    uint32_t count() const { return count_; }
    uint32_t max_count() const { return max_count_; }
    const std::list<TaskId>& wait_queue() const { return wait_queue_; }

private:
    uint32_t count_;
    uint32_t max_count_;
    std::list<TaskId> wait_queue_;  // Priority-ordered
};

// Convenience alias
using BinarySemaphore = Semaphore;

}  // namespace rtos
