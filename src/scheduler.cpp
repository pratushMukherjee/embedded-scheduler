#include "rtos/scheduler.hpp"

#include <algorithm>

namespace rtos {

Scheduler::Scheduler() : config_() {}
Scheduler::Scheduler(SchedulerConfig config) : config_(config) {}

TaskId Scheduler::create_task(const std::string& name, Priority priority, TaskFunction func,
                               std::size_t stack_size) {
    TaskId id = next_id_++;
    Task task;
    task.id = id;
    task.name = name;
    task.base_priority = priority;
    task.effective_priority = priority;
    task.state = TaskState::DORMANT;
    task.func = std::move(func);
    task.stack_size = stack_size;
    task.time_slice_remaining = config_.default_time_slice;
    tasks_.push_back(std::move(task));
    return id;
}

void Scheduler::start_task(TaskId id) {
    Task* task = get_task(id);
    if (!task || task->state != TaskState::DORMANT) return;
    task->state = TaskState::READY;
    task->time_slice_remaining = config_.default_time_slice;
    add_to_ready_list(id);
}

void Scheduler::suspend_task(TaskId id) {
    Task* task = get_task(id);
    if (!task) return;
    if (task->state == TaskState::READY) {
        remove_from_ready_list(id);
    }
    task->state = TaskState::SUSPENDED;
}

void Scheduler::resume_task(TaskId id) {
    Task* task = get_task(id);
    if (!task || task->state != TaskState::SUSPENDED) return;
    task->state = TaskState::READY;
    task->time_slice_remaining = config_.default_time_slice;
    add_to_ready_list(id);
}

void Scheduler::delete_task(TaskId id) {
    Task* task = get_task(id);
    if (!task) return;
    remove_from_ready_list(id);
    task->state = TaskState::DORMANT;
    if (current_task_ == id) current_task_ = kNoTask;
}

void Scheduler::block_task(TaskId id, BlockReason reason, void* object, TickCount timeout) {
    Task* task = get_task(id);
    if (!task) return;
    if (task->state == TaskState::READY || task->state == TaskState::RUNNING) {
        remove_from_ready_list(id);
    }
    task->state = TaskState::BLOCKED;
    task->block_reason = reason;
    task->block_object = object;
    task->block_timeout = (timeout > 0) ? (tick_count_ + timeout) : 0;
}

void Scheduler::unblock_task(TaskId id) {
    Task* task = get_task(id);
    if (!task || task->state != TaskState::BLOCKED) return;
    task->state = TaskState::READY;
    task->block_reason = BlockReason::NONE;
    task->block_object = nullptr;
    task->block_timeout = 0;
    task->time_slice_remaining = config_.default_time_slice;
    add_to_ready_list(id);
}

void Scheduler::set_effective_priority(TaskId id, Priority priority) {
    Task* task = get_task(id);
    if (!task) return;
    Priority old_priority = task->effective_priority;
    task->effective_priority = priority;

    // If task is in ready list, move it to the new priority level
    if (task->state == TaskState::READY || task->state == TaskState::RUNNING) {
        // Remove from old priority list
        auto old_it = ready_lists_.find(old_priority);
        if (old_it != ready_lists_.end()) {
            old_it->second.remove(id);
            if (old_it->second.empty()) ready_lists_.erase(old_it);
        }
        // Add to new priority list
        ready_lists_[priority].push_back(id);
    }
}

void Scheduler::restore_base_priority(TaskId id) {
    Task* task = get_task(id);
    if (!task) return;
    set_effective_priority(id, task->base_priority);
}

void Scheduler::delay_task(TaskId id, TickCount ticks) {
    Task* task = get_task(id);
    if (!task) return;
    if (task->state == TaskState::READY || task->state == TaskState::RUNNING) {
        remove_from_ready_list(id);
    }
    task->state = TaskState::BLOCKED;
    task->block_reason = BlockReason::DELAY;
    task->wake_tick = tick_count_ + ticks;
}

void Scheduler::yield_task(TaskId id) {
    Task* task = get_task(id);
    if (!task) return;
    task->state = TaskState::READY;
    task->time_slice_remaining = config_.default_time_slice;
    // Move to back of ready list
    auto it = ready_lists_.find(task->effective_priority);
    if (it != ready_lists_.end()) {
        it->second.remove(id);
        it->second.push_back(id);
    }
}

TaskId Scheduler::schedule() {
    // Find highest priority ready task (lowest numerical priority = highest)
    for (auto& [priority, list] : ready_lists_) {
        if (!list.empty()) {
            TaskId next = list.front();
            current_task_ = next;
            return next;
        }
    }
    current_task_ = kNoTask;
    return kNoTask;
}

void Scheduler::tick() {
    ++tick_count_;

    // Wake delayed tasks
    for (auto& task : tasks_) {
        if (task.state == TaskState::BLOCKED && task.block_reason == BlockReason::DELAY) {
            if (tick_count_ > task.wake_tick) {
                task.state = TaskState::READY;
                task.block_reason = BlockReason::NONE;
                task.time_slice_remaining = config_.default_time_slice;
                add_to_ready_list(task.id);
            }
        }
    }

    // Check blocked task timeouts
    for (auto& task : tasks_) {
        if (task.state == TaskState::BLOCKED && task.block_reason != BlockReason::DELAY &&
            task.block_timeout > 0 && tick_count_ >= task.block_timeout) {
            unblock_task(task.id);
        }
    }

    // Decrement current task's time slice
    Task* current = get_task(current_task_);
    if (current && current->state == TaskState::RUNNING) {
        if (current->time_slice_remaining > 0) {
            --current->time_slice_remaining;
        }
        // Round-robin: if time slice expired, move to back of ready list
        if (current->time_slice_remaining == 0) {
            current->state = TaskState::READY;
            current->time_slice_remaining = config_.default_time_slice;
            // Move to back of its priority list
            auto it = ready_lists_.find(current->effective_priority);
            if (it != ready_lists_.end()) {
                it->second.remove(current_task_);
                it->second.push_back(current_task_);
            } else {
                add_to_ready_list(current_task_);
            }
        }
    }
}

Task* Scheduler::get_task(TaskId id) {
    for (auto& task : tasks_) {
        if (task.id == id) return &task;
    }
    return nullptr;
}

const Task* Scheduler::get_task(TaskId id) const {
    for (const auto& task : tasks_) {
        if (task.id == id) return &task;
    }
    return nullptr;
}

void Scheduler::add_to_ready_list(TaskId id) {
    Task* task = get_task(id);
    if (!task) return;
    auto& list = ready_lists_[task->effective_priority];
    // Avoid duplicates
    if (std::find(list.begin(), list.end(), id) == list.end()) {
        list.push_back(id);
    }
}

void Scheduler::remove_from_ready_list(TaskId id) {
    Task* task = get_task(id);
    if (!task) return;
    auto it = ready_lists_.find(task->effective_priority);
    if (it != ready_lists_.end()) {
        it->second.remove(id);
        if (it->second.empty()) {
            ready_lists_.erase(it);
        }
    }
}

}  // namespace rtos
