#include "rtos/sync/mutex.hpp"
#include "rtos/scheduler.hpp"

namespace rtos {

bool Mutex::try_lock(TaskId caller) {
    if (owner_ == kNoTask) {
        owner_ = caller;
        return true;
    }
    if (owner_ == caller) {
        return true;  // Recursive lock (same owner)
    }
    return false;
}

bool Mutex::lock(Scheduler& sched, TaskId caller, TickCount timeout) {
    if (try_lock(caller)) return true;

    Task* caller_task = sched.get_task(caller);
    if (!caller_task) return false;

    // Priority inheritance: boost owner to caller's priority if higher
    Task* owner_task = sched.get_task(owner_);
    if (owner_task && caller_task->effective_priority < owner_task->effective_priority) {
        original_priority_ = owner_task->effective_priority;
        sched.set_effective_priority(owner_, caller_task->effective_priority);
    }

    // Insert into wait queue sorted by effective priority
    auto it = wait_queue_.begin();
    while (it != wait_queue_.end()) {
        Task* waiting = sched.get_task(*it);
        if (waiting && waiting->effective_priority > caller_task->effective_priority) {
            break;
        }
        ++it;
    }
    wait_queue_.insert(it, caller);
    sched.block_task(caller, BlockReason::MUTEX, this, timeout);
    return false;
}

void Mutex::unlock(Scheduler& sched) {
    if (owner_ == kNoTask) return;

    // Restore owner's base priority
    sched.restore_base_priority(owner_);
    owner_ = kNoTask;

    if (!wait_queue_.empty()) {
        // Transfer ownership to highest priority waiter
        TaskId waiter = wait_queue_.front();
        wait_queue_.pop_front();
        owner_ = waiter;

        Task* waiter_task = sched.get_task(waiter);
        if (waiter_task) {
            original_priority_ = waiter_task->effective_priority;
        }

        sched.unblock_task(waiter);
    }
}

}  // namespace rtos
