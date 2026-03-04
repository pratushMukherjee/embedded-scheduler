#pragma once

#include "rtos/task.hpp"

#include <list>
#include <map>
#include <optional>
#include <vector>

namespace rtos {

struct SchedulerConfig {
    TickCount default_time_slice{10};
    TickCount tick_rate_hz{1000};
};

class Scheduler {
public:
    Scheduler();
    explicit Scheduler(SchedulerConfig config);

    // Task management
    TaskId create_task(const std::string& name, Priority priority, TaskFunction func,
                       std::size_t stack_size = kDefaultStackSize);
    void start_task(TaskId id);
    void suspend_task(TaskId id);
    void resume_task(TaskId id);
    void delete_task(TaskId id);

    // Block/unblock (called by sync primitives)
    void block_task(TaskId id, BlockReason reason, void* object, TickCount timeout);
    void unblock_task(TaskId id);

    // Priority management
    void set_effective_priority(TaskId id, Priority priority);
    void restore_base_priority(TaskId id);

    // Delay
    void delay_task(TaskId id, TickCount ticks);

    // Scheduling
    TaskId schedule();
    void tick();

    // Access
    Task* get_task(TaskId id);
    const Task* get_task(TaskId id) const;
    TaskId current_task_id() const { return current_task_; }
    TickCount tick_count() const { return tick_count_; }
    const std::vector<Task>& all_tasks() const { return tasks_; }
    const SchedulerConfig& config() const { return config_; }

private:
    void add_to_ready_list(TaskId id);
    void remove_from_ready_list(TaskId id);

    SchedulerConfig config_;
    std::vector<Task> tasks_;
    TaskId current_task_{kNoTask};
    TaskId next_id_{1};
    std::map<Priority, std::list<TaskId>> ready_lists_;
    TickCount tick_count_{0};
};

}  // namespace rtos
