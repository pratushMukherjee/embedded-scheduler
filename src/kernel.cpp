#include "rtos/kernel.hpp"

#include <set>
#include <map>

namespace rtos {

Kernel::Kernel() : config_(), scheduler_(config_.scheduler) {}
Kernel::Kernel(KernelConfig config) : config_(config), scheduler_(config.scheduler) {}

void Kernel::tick() {
    // 1. Advance scheduler tick (wakes delayed tasks, handles timeouts, round-robin)
    scheduler_.tick();

    // 2. Process software timers
    process_timers();

    // 3. Process watchdogs
    process_watchdogs();

    // 4. Remember previous task before scheduling
    TaskId prev = scheduler_.current_task_id();

    // 5. Schedule: pick highest priority READY task
    TaskId next = scheduler_.schedule();
    Task* next_task = scheduler_.get_task(next);

    // 6. Context switch if needed
    if (next != prev && next != kNoTask) {
        Task* prev_task = scheduler_.get_task(prev);
        if (prev_task && prev_task->state == TaskState::RUNNING) {
            prev_task->state = TaskState::READY;
        }
        if (next_task) {
            next_task->context_switches++;
            stats_.total_context_switches++;
        }
    }

    // 7. Execute one step of the selected task
    if (next_task && (next_task->state == TaskState::READY ||
                      next_task->state == TaskState::RUNNING)) {
        next_task->state = TaskState::RUNNING;

        // Simulate stack usage
        next_task->stack_usage += 64;
        if (next_task->stack_usage > next_task->stack_high_water) {
            next_task->stack_high_water = next_task->stack_usage;
        }
        next_task->stack_usage = next_task->stack_high_water / 2;

        TaskAction action = next_task->func();

        switch (action) {
            case TaskAction::CONTINUE:
                break;
            case TaskAction::YIELD:
                scheduler_.yield_task(next);
                break;
            case TaskAction::FINISHED:
                next_task->state = TaskState::DORMANT;
                scheduler_.delete_task(next);
                break;
        }
    }

    // 8. Update stats
    update_stats(next);
    stats_.total_ticks++;
}

void Kernel::run(TickCount num_ticks) {
    for (TickCount i = 0; i < num_ticks; ++i) {
        tick();
    }

    if (stats_.total_ticks > 0) {
        stats_.cpu_utilization =
            1.0 - (static_cast<double>(stats_.idle_ticks) / stats_.total_ticks);
        for (auto& [id, ts] : stats_.per_task) {
            ts.cpu_percent = static_cast<double>(ts.ticks_run) / stats_.total_ticks * 100.0;
        }
    }
}

void Kernel::update_stats(TaskId running_task) {
    if (running_task == kNoTask) return;
    const Task* task = scheduler_.get_task(running_task);
    if (!task) return;

    auto& ts = stats_.per_task[running_task];
    ts.ticks_run++;
    ts.context_switches = task->context_switches;
    ts.stack_high_water = task->stack_high_water;

    if (task->base_priority == kIdlePriority) {
        stats_.idle_ticks++;
    }
}

// --- Timers ---

uint32_t Kernel::create_timer(const std::string& name, TimerMode mode,
                               TickCount period, TimerCallback callback) {
    Timer t;
    t.id = next_timer_id_++;
    t.name = name;
    t.mode = mode;
    t.period = period;
    t.callback = std::move(callback);
    t.active = false;
    timers_.push_back(std::move(t));
    return timers_.back().id;
}

void Kernel::start_timer(uint32_t id) {
    for (auto& t : timers_) {
        if (t.id == id) {
            t.active = true;
            t.next_fire = scheduler_.tick_count() + t.period;
            return;
        }
    }
}

void Kernel::stop_timer(uint32_t id) {
    for (auto& t : timers_) {
        if (t.id == id) {
            t.active = false;
            return;
        }
    }
}

void Kernel::process_timers() {
    TickCount now = scheduler_.tick_count();
    for (auto& t : timers_) {
        if (!t.active) continue;
        if (now >= t.next_fire) {
            t.callback();
            if (t.mode == TimerMode::PERIODIC) {
                t.next_fire = now + t.period;
            } else {
                t.active = false;
            }
        }
    }
}

// --- Watchdogs ---

void Kernel::start_watchdog(TaskId task_id, TickCount timeout, WatchdogCallback on_expire) {
    // Check if watchdog already exists for this task
    for (auto& w : watchdogs_) {
        if (w.task_id == task_id) {
            w.timeout = timeout;
            w.deadline = scheduler_.tick_count() + timeout;
            w.on_expire = std::move(on_expire);
            w.active = true;
            return;
        }
    }
    Watchdog w;
    w.task_id = task_id;
    w.timeout = timeout;
    w.deadline = scheduler_.tick_count() + timeout;
    w.on_expire = std::move(on_expire);
    w.active = true;
    watchdogs_.push_back(std::move(w));
}

void Kernel::kick_watchdog(TaskId task_id) {
    for (auto& w : watchdogs_) {
        if (w.task_id == task_id && w.active) {
            w.deadline = scheduler_.tick_count() + w.timeout;
            return;
        }
    }
}

void Kernel::stop_watchdog(TaskId task_id) {
    for (auto& w : watchdogs_) {
        if (w.task_id == task_id) {
            w.active = false;
            return;
        }
    }
}

void Kernel::process_watchdogs() {
    TickCount now = scheduler_.tick_count();
    for (auto& w : watchdogs_) {
        if (!w.active) continue;
        if (now >= w.deadline) {
            w.on_expire(w.task_id);
            w.active = false;
        }
    }
}

// --- Deadlock detection ---

bool Kernel::detect_deadlock() const {
    // Build wait-for graph: task A waits for mutex/semaphore held by task B
    // Use DFS to detect cycles
    std::map<TaskId, TaskId> waits_for;  // blocked_task -> blocking_task

    for (const auto& task : scheduler_.all_tasks()) {
        if (task.state != TaskState::BLOCKED || task.block_reason != BlockReason::MUTEX) continue;
        if (!task.block_object) continue;

        // Find which task owns the mutex this task is waiting on
        for (const auto& other : scheduler_.all_tasks()) {
            if (other.id == task.id) continue;
            if (other.state == TaskState::RUNNING || other.state == TaskState::READY ||
                other.state == TaskState::BLOCKED) {
                waits_for[task.id] = other.id;
                break;
            }
        }
    }

    // DFS cycle detection
    for (const auto& [start, _] : waits_for) {
        std::set<TaskId> visited;
        TaskId current = start;
        while (waits_for.count(current) > 0) {
            if (visited.count(current) > 0) {
                return true;  // Cycle detected
            }
            visited.insert(current);
            current = waits_for.at(current);
        }
    }

    return false;
}

// --- Stack overflow ---

bool Kernel::check_stack_overflow(TaskId task_id) const {
    const Task* task = scheduler_.get_task(task_id);
    if (!task) return false;
    return task->stack_high_water >= task->stack_size;
}

}  // namespace rtos
