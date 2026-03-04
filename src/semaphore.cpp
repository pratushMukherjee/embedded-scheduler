#include "rtos/sync/semaphore.hpp"
#include "rtos/scheduler.hpp"

#include <algorithm>

namespace rtos {

Semaphore::Semaphore(uint32_t initial_count, uint32_t max_count)
    : count_(initial_count), max_count_(max_count) {}

bool Semaphore::try_take() {
    if (count_ > 0) {
        --count_;
        return true;
    }
    return false;
}

bool Semaphore::take(Scheduler& sched, TaskId caller, TickCount timeout) {
    if (try_take()) return true;

    // Block the caller in priority order
    Task* task = sched.get_task(caller);
    if (!task) return false;

    // Insert into wait queue sorted by effective priority (lower = higher priority)
    auto it = wait_queue_.begin();
    while (it != wait_queue_.end()) {
        Task* waiting = sched.get_task(*it);
        if (waiting && waiting->effective_priority > task->effective_priority) {
            break;
        }
        ++it;
    }
    wait_queue_.insert(it, caller);
    sched.block_task(caller, BlockReason::SEMAPHORE, this, timeout);
    return false;
}

void Semaphore::give(Scheduler& sched) {
    if (!wait_queue_.empty()) {
        // Wake highest priority waiter
        TaskId waiter = wait_queue_.front();
        wait_queue_.pop_front();
        sched.unblock_task(waiter);
        // Semaphore count stays 0 — token goes directly to waiter
    } else if (count_ < max_count_) {
        ++count_;
    }
}

}  // namespace rtos
