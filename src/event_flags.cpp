#include "rtos/sync/event_flags.hpp"
#include "rtos/scheduler.hpp"

namespace rtos {

bool EventFlags::check(uint32_t requested, EventWaitMode mode) const {
    if (mode == EventWaitMode::ANY) {
        return (flags_ & requested) != 0;
    }
    return (flags_ & requested) == requested;
}

void EventFlags::set(Scheduler& sched, uint32_t flags) {
    flags_ |= flags;

    // Wake any waiters whose conditions are now met
    auto it = waiters_.begin();
    while (it != waiters_.end()) {
        if (check(it->requested_flags, it->mode)) {
            TaskId id = it->task_id;
            it = waiters_.erase(it);
            sched.unblock_task(id);
        } else {
            ++it;
        }
    }
}

void EventFlags::clear(uint32_t flags) {
    flags_ &= ~flags;
}

bool EventFlags::wait(Scheduler& sched, TaskId caller, uint32_t requested,
                      EventWaitMode mode, TickCount timeout) {
    if (check(requested, mode)) return true;

    waiters_.push_back({caller, requested, mode});
    sched.block_task(caller, BlockReason::EVENT_FLAGS, this, timeout);
    return false;
}

}  // namespace rtos
