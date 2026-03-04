#include "rtos/kernel.hpp"

namespace rtos {

Kernel::Kernel() : config_(), scheduler_(config_.scheduler) {}
Kernel::Kernel(KernelConfig config) : config_(config), scheduler_(config.scheduler) {}

void Kernel::tick() {
    // 1. Advance scheduler tick (wakes delayed tasks, handles timeouts, round-robin)
    scheduler_.tick();

    // 2. Remember previous task before scheduling
    TaskId prev = scheduler_.current_task_id();

    // 3. Schedule: pick highest priority READY task
    TaskId next = scheduler_.schedule();
    Task* next_task = scheduler_.get_task(next);

    // 4. Context switch if needed
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

    // 5. Execute one step of the selected task
    if (next_task && (next_task->state == TaskState::READY ||
                      next_task->state == TaskState::RUNNING)) {
        next_task->state = TaskState::RUNNING;

        // Simulate stack usage
        next_task->stack_usage += 64;
        if (next_task->stack_usage > next_task->stack_high_water) {
            next_task->stack_high_water = next_task->stack_usage;
        }
        // Reset stack usage each step (simulating function call/return)
        next_task->stack_usage = next_task->stack_high_water / 2;

        TaskAction action = next_task->func();

        switch (action) {
            case TaskAction::CONTINUE:
                // Stay running
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

    // 6. Update stats
    update_stats(next);
    stats_.total_ticks++;
}

void Kernel::run(TickCount num_ticks) {
    for (TickCount i = 0; i < num_ticks; ++i) {
        tick();
    }

    // Calculate final CPU percentages
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

}  // namespace rtos
